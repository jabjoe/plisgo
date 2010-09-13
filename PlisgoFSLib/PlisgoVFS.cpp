/*
	Copyright 2009 Eurocom Entertainment Software Limited

    This file is part of PlisgoFSLib.

    Eurocom Entertainment Software Limited, hereby disclaims all
    copyright interest in “PlisgoFSLib” written by Joe Burmeister.

    PlisgoFSLib is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

    PlisgoFSLib is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
	License along with PlisgoFSLib.  If not, see
	<http://www.gnu.org/licenses/>.
*/

#include "PlisgoVFS.h"

PlisgoVFS::PlisgoVFS(IPtrPlisgoFSFolder root, PlisgoVFSOpenLog* pLog)
{
	assert(root.get() != NULL);
	m_Root = root;
	m_OpenFileNum = 0;
	m_pLog = pLog;
	m_pLatestOpen = NULL;
}


bool				PlisgoVFS::AddMount(LPCWSTR sMount, IPtrPlisgoFSFile Mount)
{
	assert(sMount != NULL && Mount.get() != NULL);

	std::wstring sPathLowerCase = sMount;

	MakePathHashSafe(sPathLowerCase);

	IPtrPlisgoFSFile existing = TracePath(sPathLowerCase);

	if (existing.get() == NULL)
		return false;

	boost::unique_lock<boost::shared_mutex> cacheLock(m_CacheEntryMutex);

	m_MountTree.SetData(sPathLowerCase, Mount);

	RestartCache();

	return true;
}


bool				PlisgoVFS::RemoveMount(LPCWSTR sMount)
{
	std::wstring sPathLowerCase = sMount;

	MakePathHashSafe(sPathLowerCase);

	IPtrPlisgoFSFile existing = TracePath(sPathLowerCase);

	if (existing.get() == NULL)
		return false;

	boost::unique_lock<boost::shared_mutex> cacheLock(m_CacheEntryMutex);

	m_MountTree.RemoveAndPrune(sPathLowerCase, false);

	RestartCache();

	return true;
}


IPtrPlisgoFSFile	PlisgoVFS::GetMount(LPCWSTR sMount)
{
	std::wstring sPathLowerCase = sMount;

	MakePathHashSafe(sPathLowerCase);

	IPtrPlisgoFSFile result;

	m_MountTree.GetData(sPathLowerCase, result);

	return result;
}

	
void				PlisgoVFS::GetMounts(std::map<std::wstring,IPtrPlisgoFSFile>& rMounts)
{
	MountTree::FullKeyMap	mounts;

	m_MountTree.GetFullKeyMap(mounts);

	for(MountTree::FullKeyMap::const_iterator it = mounts.begin(); it != mounts.end(); ++it)
		rMounts[it->first] = it->second;
}



bool				PlisgoVFS::GetCached(const std::wstring& rsPath, IPtrPlisgoFSFile& rFile) const
{
	boost::upgrade_lock<boost::shared_mutex> readLock(m_CacheEntryMutex);

	CacheEntryMap::const_iterator it = m_CacheEntryMap.find(rsPath);

	if (it == m_CacheEntryMap.end())
		return false;

	const Cached& rCached = it->second;

	rFile = rCached.file;

	assert(rFile.get() != NULL); //We don't do duds

	if (!rFile->IsValid())
	{
		boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock(readLock);

		//Use key instead of iterator in case been cleared already between read/write lock transition.
		const_cast<PlisgoVFS*>(this)->m_CacheEntryMap.erase(rsPath);

		return false;
	}
	else
	{
		//This might be worth returning to. Aging entries so we can flush them out.
/*
		ULONG64	nNow;

		GetSystemTimeAsFileTime((FILETIME*)&nNow);
*/
	}
	
	return true;
}


IPtrPlisgoFSFile	PlisgoVFS::TracePath(const std::wstring& rsLowerPath, IPtrPlisgoFSFile* pParent) const
{
	if (rsLowerPath.length() == 0)
		return m_Root;

	IPtrPlisgoFSFile file;

	if (pParent == NULL)
		if (GetCached(rsLowerPath, file))
			return file;

	IPtrPlisgoFSFile parent;

	size_t nSlash = rsLowerPath.rfind(L'\\');

	if (nSlash != -1)
	{
		parent = TracePath(rsLowerPath.substr(0, nSlash));

		if (parent.get() == NULL)
			return IPtrPlisgoFSFile();
	}
	else parent = m_Root;

	if (pParent != NULL)
	{
		*pParent = parent;

		//If pParent was NULL we would have done this already
		if (GetCached(rsLowerPath, file))
			return file;
	}
	
	PlisgoFSFolder*	pFolder = parent->GetAsFolder();

	if (pFolder != NULL)
	{
		file = pFolder->GetChild(&rsLowerPath.c_str()[nSlash+1]);

		const_cast<PlisgoVFS*>(this)->AddToCache(rsLowerPath, file);
	}

	return file;
}


IPtrPlisgoFSFile	PlisgoVFS::TracePath(LPCWSTR sPath, IPtrPlisgoFSFile* pParent) const
{
	if (sPath == NULL || sPath[0] == L'\0')
		return m_Root;

	if (sPath[0] != L'\\')
		return IPtrPlisgoFSFile();

	++sPath;

	if (sPath[0] == L'\0')
		return m_Root;

	std::wstring sPathLowerCase = sPath;

	MakePathHashSafe(sPathLowerCase);

	return TracePath(sPathLowerCase, pParent);
}


int					PlisgoVFS::GetChildren(LPCWSTR sPath, PlisgoFSFolder::ChildNames& rChildren) const
{
	std::wstring sPathLowerCase = sPath;

	MakePathHashSafe(sPathLowerCase);

	IPtrPlisgoFSFile	file = TracePath(sPathLowerCase);

	if (file.get() == NULL)
		return -ERROR_BAD_PATHNAME;

	PlisgoFSFolder* pFolder = file->GetAsFolder();

	if (pFolder == NULL)
		return -ERROR_BAD_PATHNAME;

	return pFolder->GetChildren(rChildren);
}


int					PlisgoVFS::GetChildren(PlisgoFileHandle& rHandle, PlisgoFSFolder::ChildNames& rChildren) const
{
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return -ERROR_INVALID_HANDLE;

	PlisgoFSFolder* pFolder = pOpenFileData->File->GetAsFolder();

	if (pFolder == NULL)
		return -ERROR_BAD_PATHNAME;

	return pFolder->GetChildren(rChildren);
}


void				PlisgoVFS::MoveMounts(std::wstring sOldPath, std::wstring sNewPath)
{
	boost::trim_right_if(sOldPath, boost::is_any_of(L"\\"));
	std::transform(sOldPath.begin(),sOldPath.end(),sOldPath.begin(),tolower);

	boost::trim_right_if(sNewPath, boost::is_any_of(L"\\"));
	std::transform(sNewPath.begin(),sNewPath.end(),sNewPath.begin(),tolower);

	m_MountTree.MoveBranch(sOldPath, sNewPath);
}


int					PlisgoVFS::Repath(LPCWSTR sOldPath, LPCWSTR sNewPath, bool bReplaceExisting)
{
	IPtrPlisgoFSFile newParent;
	IPtrPlisgoFSFile newFile = TracePath(sNewPath, &newParent);

	if (newParent.get() == NULL)
		return -ERROR_BAD_PATHNAME;

	PlisgoFSFolder* pNewParent = newParent->GetAsFolder();

	assert(pNewParent != NULL);

	if (newFile.get() != NULL && !bReplaceExisting)
		return -ERROR_ALREADY_EXISTS;

	IPtrPlisgoFSFile oldParent;
	IPtrPlisgoFSFile oldFile = TracePath(sOldPath, &oldParent);

	if (oldFile.get() == NULL)
		return -ERROR_FILE_NOT_FOUND;

	if (oldParent.get() == NULL)
		return -ERROR_FILE_NOT_FOUND; //wtf? Should never happen

	PlisgoFSFolder* pOldParent = oldParent->GetAsFolder();

	//Get the cache entry lock as we will need to nuke the cache
	boost::unique_lock<boost::shared_mutex> cacheLock(m_CacheEntryMutex);

	int nError = pOldParent->Repath(GetNameFromPath(sOldPath),
									GetNameFromPath(sNewPath),
									bReplaceExisting, pNewParent);

	if (nError != 0)
		return nError;

	//Update the mount table
	MoveMounts(sOldPath, sNewPath);

	//Nuke the cache
	RestartCache();

	return 0;
}


void				PlisgoVFS::GetOpenFilePath(PlisgoFileHandle& rHandle, std::wstring& rsPath)
{
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return;

	rsPath = pOpenFileData->sPath;
}


IPtrPlisgoFSFile	PlisgoVFS::GetParent(PlisgoFileHandle& rHandle) const
{
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return IPtrPlisgoFSFile();

	const std::wstring& rsPath = pOpenFileData->sPath;

	if (rsPath.length() == 0)
		return IPtrPlisgoFSFile(); //Is root

	size_t nSlash = rsPath.rfind(L'\\');

	if (nSlash == -1)
		return m_Root;

	std::wstring sParent(rsPath.begin(), rsPath.begin()+nSlash);

	return TracePath(sParent);
}


int					PlisgoVFS::GetChild(IPtrPlisgoFSFile& rChild, PlisgoFileHandle& rHandle, LPCWSTR sChildName) const
{
	if (sChildName == NULL)
		return -ERROR_INVALID_HANDLE;

	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return -ERROR_INVALID_HANDLE;

	PlisgoFSFolder* pFolder = pOpenFileData->File->GetAsFolder();

	if (pFolder == NULL)
		return -ERROR_BAD_PATHNAME;

	size_t			nChildNameLen	= wcslen(sChildName);
	std::wstring	sLowerPath		= pOpenFileData->sPath;

	if (sLowerPath.length())
		sLowerPath += L"\\";

	sLowerPath.append(nChildNameLen,L' ');

	CopyToLower((WCHAR*)sLowerPath.c_str() + pOpenFileData->sPath.length()+1, nChildNameLen+1, sChildName);

	if (!GetCached(sLowerPath, rChild))
	{
		rChild = pFolder->GetChild(sChildName);

		const_cast<PlisgoVFS*>(this)->AddToCache(sLowerPath, rChild);
	}

	if (rChild.get() == NULL)
		return -ERROR_FILE_NOT_FOUND;

	return 0;
}


IPtrPlisgoFSFile	PlisgoVFS::GetPlisgoFSFile(PlisgoFileHandle& rHandle) const
{
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return IPtrPlisgoFSFile();

	return pOpenFileData->File;
}



int					PlisgoVFS::Open(	PlisgoFileHandle&	rHandle,
										LPCWSTR				sPath,
										DWORD				nDesiredAccess,
										DWORD				nShareMode,
										DWORD				nCreationDisposition,
										DWORD				nFlagsAndAttributes)
{
	std::wstring sPathLowerCase = sPath;

	MakePathHashSafe(sPathLowerCase);

	IPtrPlisgoFSFile	file = TracePath(sPathLowerCase);

	int nError = 0;
	ULONG64 nOpenInstaceData = 0;

	{
		boost::shared_lock<boost::shared_mutex> lock(m_OpenFilePoolMutex);

		if (m_PendingDeletes.find(sPathLowerCase) != m_PendingDeletes.end())
			return -ERROR_DELETE_PENDING;
	}

	if (file.get() == NULL)
	{
		if (nCreationDisposition != OPEN_EXISTING && nCreationDisposition != TRUNCATE_EXISTING)
		{
			size_t nSlash = sPathLowerCase.rfind(L'\\');

			nSlash = (nSlash == -1)?0:nSlash;

			IPtrPlisgoFSFile parent = TracePath(sPathLowerCase.substr(0, nSlash));

			if (parent.get() != NULL)
			{
				PlisgoFSFolder* pFolder = parent->GetAsFolder();

				if (pFolder != NULL)
				{
					nError = pFolder->CreateChild(file, GetNameFromPath(sPath), nFlagsAndAttributes, &nOpenInstaceData);

					if (nError != 0)
						return nError;

					assert(file.get() != NULL);

					AddToCache(sPathLowerCase, file);
				}
				else nError = -ERROR_PATH_NOT_FOUND; //wtf, silly user
			}
			else nError = -ERROR_PATH_NOT_FOUND;
		}
		else nError = -ERROR_FILE_NOT_FOUND;
	}
	else nError = file->Open(nDesiredAccess, nShareMode, nCreationDisposition, nFlagsAndAttributes, &nOpenInstaceData);

	if (nError != 0)
	{		
		if (m_pLog != NULL)
			m_pLog->OpenFileFailed(sPathLowerCase, nError);

		return nError;
	}

	assert(file.get() != NULL);

	boost::unique_lock<boost::shared_mutex>	lock(m_OpenFilePoolMutex);
	
	OpenFileData* pOpenFileData = m_OpenFilePool.construct();

	if (pOpenFileData == NULL)
	{
		file->Close(&nOpenInstaceData);

		if (m_pLog != NULL)
			m_pLog->OpenFileFailed(sPathLowerCase, -ERROR_TOO_MANY_OPEN_FILES);

		return -ERROR_TOO_MANY_OPEN_FILES;
	}

	InterlockedIncrement(&m_OpenFileNum);

	pOpenFileData->File = file;

	pOpenFileData->sPath = sPathLowerCase;
	pOpenFileData->nData = nOpenInstaceData;
	
	if (nFlagsAndAttributes&FILE_FLAG_DELETE_ON_CLOSE)
	{
		pOpenFileData->bDeleteOnClose = true;
		m_PendingDeletes[sPathLowerCase] = true;
	}
	else pOpenFileData->bDeleteOnClose = false;

	pOpenFileData->pPrev = NULL;

	if (m_pLatestOpen != NULL)
	{
		pOpenFileData->pNext = m_pLatestOpen;
		m_pLatestOpen->pPrev = pOpenFileData;
	}
	else pOpenFileData->pNext = NULL;

	m_pLatestOpen = pOpenFileData;

	if (m_pLog != NULL)
		m_pLog->OpenFile(sPathLowerCase);

	rHandle = (PlisgoFileHandle)pOpenFileData;

	return 0;
}



PlisgoVFS::OpenFileData*		PlisgoVFS::GetOpenFileData(PlisgoFileHandle&	rHandle) const
{
	OpenFileData* pOpenFileData = (OpenFileData*)rHandle;

	boost::shared_lock<boost::shared_mutex>	lock(m_OpenFilePoolMutex);

	if (!m_OpenFilePool.is_from(pOpenFileData))
		return NULL;

	return pOpenFileData;
}


int					PlisgoVFS::Read(	PlisgoFileHandle&	rHandle,
										LPVOID				pBuffer,
										DWORD				nNumberOfBytesToRead,
										LPDWORD				pnNumberOfBytesRead,
										LONGLONG			nOffset)
{
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return -ERROR_INVALID_HANDLE;

	return pOpenFileData->File->Read(	pBuffer,
										nNumberOfBytesToRead,
										pnNumberOfBytesRead,
										nOffset,
										&pOpenFileData->nData);
}


int					PlisgoVFS::Write(	PlisgoFileHandle&	rHandle,
										LPCVOID				pBuffer,
										DWORD				nNumberOfBytesToWrite,
										LPDWORD				pnNumberOfBytesWritten,
										LONGLONG			nOffset)
{
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return -ERROR_INVALID_HANDLE;

	return pOpenFileData->File->Write(	pBuffer,
										nNumberOfBytesToWrite,
										pnNumberOfBytesWritten,
										nOffset,
										&pOpenFileData->nData);
}


int					PlisgoVFS::LockFile(PlisgoFileHandle&	rHandle,
										LONGLONG			nByteOffset,
										LONGLONG			nByteLength)
{
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return -ERROR_INVALID_HANDLE;

	return pOpenFileData->File->LockFile(	nByteOffset,
											nByteLength,
											&pOpenFileData->nData);
}


int					PlisgoVFS::UnlockFile(	PlisgoFileHandle&	rHandle,
											LONGLONG			nByteOffset,
											LONGLONG			nByteLength)
{
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return -ERROR_INVALID_HANDLE;

	return pOpenFileData->File->UnlockFile(	nByteOffset,
											nByteLength,
											&pOpenFileData->nData);
}


int					PlisgoVFS::FlushBuffers(PlisgoFileHandle&	rHandle)
{
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return -ERROR_INVALID_HANDLE;

	return pOpenFileData->File->FlushBuffers(&pOpenFileData->nData);
}


int					PlisgoVFS::SetEndOfFile(PlisgoFileHandle&	rHandle, LONGLONG nEndPos)
{
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return -ERROR_INVALID_HANDLE;

	return pOpenFileData->File->SetEndOfFile(nEndPos, &pOpenFileData->nData);
}


int					PlisgoVFS::Close(PlisgoFileHandle&	rHandle, bool bDeleteOnClose)
{
	if (rHandle == 0)
		return 0; //Already closed

	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return -ERROR_INVALID_HANDLE;

	int nError = pOpenFileData->File->Close(&pOpenFileData->nData);

	const std::wstring& rsPath = pOpenFileData->sPath;

	if (nError == 0 && bDeleteOnClose)
	{
		size_t nSlash = rsPath.rfind(L'\\');

		nSlash = (nSlash == -1)?0:nSlash;

		IPtrPlisgoFSFile parent = TracePath(rsPath.substr(0,nSlash));

		assert(parent.get() != NULL);

		PlisgoFSFolder* pFolder = parent->GetAsFolder();

		assert(pFolder != NULL);

		{
			boost::unique_lock<boost::shared_mutex> lock(m_CacheEntryMutex);

			nError = pFolder->RemoveChild(GetNameFromPath(pOpenFileData->sPath.c_str()));

			if (nError == 0)
			{
				m_CacheEntryMap.erase(pOpenFileData->sPath);

				m_MountTree.RemoveBranch(pOpenFileData->sPath);

				if (pOpenFileData->File->GetAsFolder() != NULL)
					RestartCache();
			}
		}	
	}

	boost::unique_lock<boost::shared_mutex>	lock(m_OpenFilePoolMutex);

	if (m_pLog != NULL)
		m_pLog->CloseFile(pOpenFileData->sPath);

	if (m_pLatestOpen == pOpenFileData)
	{
		m_pLatestOpen = pOpenFileData->pNext;

		if (m_pLatestOpen != NULL)
			m_pLatestOpen->pPrev = NULL;
	}
	else
	{
		if (pOpenFileData->pNext != NULL)
			pOpenFileData->pNext->pPrev = pOpenFileData->pPrev;

		if (pOpenFileData->pPrev != NULL)
			pOpenFileData->pPrev->pNext = pOpenFileData->pNext;
	}

	if (pOpenFileData->bDeleteOnClose)
		m_PendingDeletes.erase(rsPath);

	m_OpenFilePool.destroy(pOpenFileData);

	InterlockedDecrement(&m_OpenFileNum);
	
	rHandle = 0;

	return nError;
}


int					PlisgoVFS::GetDeleteError(PlisgoFileHandle&	rHandle) const
{
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return -ERROR_INVALID_HANDLE;

	const std::wstring sPath = pOpenFileData->sPath;

	size_t nSlash = sPath.rfind(L'\\');

	nSlash = (nSlash == -1)?0:nSlash;

	IPtrPlisgoFSFile parent = TracePath(sPath.substr(0, nSlash));

	assert(parent.get() != NULL);

	PlisgoFSFolder* pFolder = parent->GetAsFolder();

	assert(pFolder != NULL);

	boost::unique_lock<boost::shared_mutex>	lock(m_OpenFilePoolMutex);

	int nError = pFolder->GetRemoveChildError(GetNameFromPath(pOpenFileData->sPath));

	if (nError != 0)
		return nError;

	nError = pOpenFileData->File->GetDeleteError(&pOpenFileData->nData);

	if (nError != 0)
		return nError;

	pOpenFileData->bDeleteOnClose = true;
	m_PendingDeletes[pOpenFileData->sPath] = true;

	return 0;
}


int					PlisgoVFS::SetFileTimes(	PlisgoFileHandle&	rHandle,
												const FILETIME*		pCreation,
												const FILETIME*		pLastAccess,
												const FILETIME*		pLastWrite)
{
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return -ERROR_INVALID_HANDLE;

	return pOpenFileData->File->SetFileTimes(pCreation, pLastAccess, pLastWrite, &pOpenFileData->nData);
}


int					PlisgoVFS::SetAttributes(	PlisgoFileHandle&	rHandle,
												DWORD				nFileAttributes)
{
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return -ERROR_INVALID_HANDLE;

	return pOpenFileData->File->SetAttributes(nFileAttributes, &pOpenFileData->nData);
}


int					PlisgoVFS::GetHandleInfo(PlisgoFileHandle&	rHandle, LPBY_HANDLE_FILE_INFORMATION pInfo)
{
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return -ERROR_INVALID_HANDLE;

	return pOpenFileData->File->GetHandleInfo(pInfo, &pOpenFileData->nData);
}

	
void				PlisgoVFS::CloseAllOpenFiles()
{
	boost::unique_lock<boost::shared_mutex>	lock(m_OpenFilePoolMutex);

	OpenFileData* pCurrent = m_pLatestOpen;

	while(pCurrent != NULL)
	{
		OpenFileData* pNext = pCurrent->pNext;

		pCurrent->File->Close(&pCurrent->nData);

		if (m_pLog != NULL)
			m_pLog->CloseFile(pCurrent->sPath);

		m_OpenFilePool.destroy(pCurrent);

		InterlockedDecrement(&m_OpenFileNum);

		pCurrent = pNext;
	}

	m_pLatestOpen = NULL;
}


void				PlisgoVFS::AddToCache(const std::wstring& rsLowerPath, IPtrPlisgoFSFile file)
{
	if (file.get() != NULL && file->IsValid())
	{
		boost::unique_lock<boost::shared_mutex> writeLock(m_CacheEntryMutex);

		Cached& rCached = m_CacheEntryMap[rsLowerPath];

		rCached.file = file;
		
		GetSystemTimeAsFileTime((FILETIME*)&rCached.nTime);
	}
}

	
void				PlisgoVFS::ClearCache()
{
	boost::unique_lock<boost::shared_mutex> writeLock(m_CacheEntryMutex);

	return RestartCache();
}


void				PlisgoVFS::RestartCache()
{
	//m_CacheEntryMutex should be write-locked

	m_CacheEntryMap.clear();

	MountTree::FullKeyMap fullKeyMap;

	m_MountTree.GetFullKeyMap(fullKeyMap);

	for(MountTree::FullKeyMap::const_iterator it = fullKeyMap.begin(); it != fullKeyMap.end(); ++it)
	{
		Cached& rCached = m_CacheEntryMap[it->first];
		
		rCached.file = it->second;
		rCached.nTime = -1; //From the future, so is never too old!
	}
}
