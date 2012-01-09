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

#define PLISGOVFS_DELETE_ON_CLOSE	0x1


PlisgoVFS::OpenFileDataFactory::~OpenFileDataFactory()
{
	CloseAll(NULL);
}


PlisgoVFS::PlisgoFileHandle	PlisgoVFS::OpenFileDataFactory::CreateNew(	const std::wstring&	rsPath,
																		IPtrPlisgoFSFile	file,
																		IPtrPlisgoFSData	data,
																		ULONG32				nFlags)
{
	OpenFileDataSP ref;

	int nResult = -1;

	if (m_FreeFiles.size())
	{
		//We go backwards as we want to move as little as possible
		for(int n = (int)m_FreeFiles.size()-1; n > 0; --n)
		{
			nResult = m_FreeFiles[n];

			if (m_Files[nResult].use_count() == 1)
			{
				ref = m_Files[nResult];

				m_FreeFiles.erase(m_FreeFiles.begin()+n);

				break;
			}
		}
	}
	
	if (ref.get() == NULL)
	{
		ref = boost::make_shared<OpenFileData>();

		if (ref.get() == NULL)
			return 0;

		nResult = (int)m_Files.size();

		m_Files.push_back(ref);

	}

	ref->sPath	= rsPath;
	ref->File	= file;
	ref->Data	= data;
	ref->nFlags	= nFlags;

	m_OpenMap[m_nOpenUnique] = nResult;
	
	return m_nOpenUnique++;
}


PlisgoVFS::OpenFileDataSP	PlisgoVFS::OpenFileDataFactory::GetOpenFile(const PlisgoFileHandle& rHandle) const
{
	std::map<ULONG64, int>::const_iterator it = m_OpenMap.find(rHandle);

	if (it == m_OpenMap.end())
		return PlisgoVFS::OpenFileDataSP();

	return m_Files[it->second];
}


void			PlisgoVFS::OpenFileDataFactory::FreeOpenFile(const PlisgoFileHandle& rHandle)
{
	std::map<ULONG64, int>::const_iterator it = m_OpenMap.find(rHandle);

	if (it != m_OpenMap.end())
	{
		OpenFileDataSP& rCurrent = m_Files[it->second];

		assert(rCurrent.get() != NULL);

		m_FreeFiles.push_back(it->second);
		m_OpenMap.erase(it);
	}
}


void			PlisgoVFS::OpenFileDataFactory::CloseAll(PlisgoVFSOpenLog* pLog)
{
	for(std::map<ULONG64, int>::const_iterator it = m_OpenMap.begin();
		it != m_OpenMap.end();)
	{
		OpenFileDataSP& rCurrent = m_Files[it->second];

		assert(rCurrent.get() != NULL);

		rCurrent->File->Close(rCurrent->Data);

		if (pLog != NULL)
			pLog->CloseFile(rCurrent->sPath);

		m_FreeFiles.push_back(it->second);
		it = m_OpenMap.erase(it);
	}
}

/*
*****************************************************************************************
					PlisgoVFS
*****************************************************************************************
*/

PlisgoVFS::PlisgoVFS(IPtrPlisgoFSFolder root, PlisgoVFSOpenLog* pLog)
{
	assert(root.get() != NULL);
	m_Root = root;
	m_pLog = pLog;

	m_pNewest = NULL;
	m_pOldest = NULL;
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


void				PlisgoVFS::RemoveCached(const std::wstring& rsKey)
{
	CacheEntryMap::iterator it = m_CacheEntryMap.find(rsKey);

	if (it == m_CacheEntryMap.end())
		return;

	RemoveCached(it->second);

	m_CacheEntryMap.erase(it);
}


bool				PlisgoVFS::GetCached(const std::wstring& rsPath, IPtrPlisgoFSFile& rFile) const
{
	boost::upgrade_lock<boost::shared_mutex> readLock(m_CacheEntryMutex);

	CacheEntryMap::const_iterator it = m_CacheEntryMap.find(rsPath);

	if (it == m_CacheEntryMap.end())
		return false;

	const Cached* pCached = it->second;

	rFile = pCached->file;

	if (rFile.get() == NULL || !rFile->IsValid())
	{
		//You will see this double check a lot, this is because things can change while getting the write lock
		boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock(readLock);

		it = m_CacheEntryMap.find(rsPath);

		if (it == m_CacheEntryMap.end())
			return false;
		
		pCached = it->second;

		rFile = pCached->file;

		if (rFile.get() == NULL || !rFile->IsValid())
		{
			const_cast<PlisgoVFS*>(this)->RemoveCached(rsPath);

			return false;
		}
		else return true; //For break point
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
	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return -ERROR_INVALID_HANDLE;

	PlisgoFSFolder* pFolder = openFileData->File->GetAsFolder();

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

	if (pNewParent == NULL)
		return -ERROR_BAD_PATHNAME;

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


bool				PlisgoVFS::GetOpenFileData(OpenFileDataSP& rResult, const PlisgoFileHandle&	rHandle) const
{
	boost::shared_lock<boost::shared_mutex>	lock(m_OpenFileMutex);

	rResult = m_OpenFileFactory.GetOpenFile(rHandle);

	return (rResult.get() != NULL);
}


void				PlisgoVFS::GetOpenFilePath(PlisgoFileHandle& rHandle, std::wstring& rsPath)
{
	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return;

	rsPath = openFileData->sPath;
}


IPtrPlisgoFSFile	PlisgoVFS::GetParent(PlisgoFileHandle& rHandle) const
{
	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return IPtrPlisgoFSFile();

	const std::wstring& rsPath = openFileData->sPath;

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

	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return -ERROR_INVALID_HANDLE;

	PlisgoFSFolder* pFolder = openFileData->File->GetAsFolder();

	if (pFolder == NULL)
		return -ERROR_BAD_PATHNAME;

	size_t			nChildNameLen	= wcslen(sChildName);
	std::wstring	sLowerPath		= openFileData->sPath;

	if (sLowerPath.length())
	{
		sLowerPath += L"\\";

		sLowerPath.append(nChildNameLen,L' ');

		CopyToLower((WCHAR*)sLowerPath.c_str() + openFileData->sPath.length()+1, nChildNameLen+1, sChildName);
	}
	else
	{
		sLowerPath.append(nChildNameLen,L' ');

		CopyToLower((WCHAR*)sLowerPath.c_str(), nChildNameLen+1, sChildName);
	}

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
	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return IPtrPlisgoFSFile();

	return openFileData->File;
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
	IPtrPlisgoFSData openInstaceData;

	{
		boost::shared_lock<boost::shared_mutex> lock(m_OpenFileMutex);

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
					nError = pFolder->CreateChild(file, GetNameFromPath(sPath), nDesiredAccess, nShareMode, nFlagsAndAttributes, openInstaceData);

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
	else nError = file->Open(nDesiredAccess, nShareMode, nCreationDisposition, nFlagsAndAttributes, openInstaceData);

	if (nError != 0)
	{		
		if (m_pLog != NULL)
			m_pLog->OpenFileFailed(sPathLowerCase, nError);

		return nError;
	}

	assert(file.get() != NULL);

	boost::unique_lock<boost::shared_mutex>	lock(m_OpenFileMutex);
	
	ULONG32 nOpenFlag = 0;

	if (nFlagsAndAttributes&FILE_FLAG_DELETE_ON_CLOSE)
	{
		nOpenFlag = PLISGOVFS_DELETE_ON_CLOSE;
		m_PendingDeletes[sPathLowerCase] = true;
	}

	rHandle = m_OpenFileFactory.CreateNew(	sPathLowerCase,
											file,
											openInstaceData,
											nOpenFlag);
	if (rHandle == 0)
	{
		file->Close(openInstaceData);

		if (m_pLog != NULL)
			m_pLog->OpenFileFailed(sPathLowerCase, -ERROR_TOO_MANY_OPEN_FILES);

		return -ERROR_TOO_MANY_OPEN_FILES;
	}

	if (m_pLog != NULL)
		m_pLog->OpenFile(sPathLowerCase);

	return 0;
}


int					PlisgoVFS::Read(	PlisgoFileHandle&	rHandle,
										LPVOID				pBuffer,
										DWORD				nNumberOfBytesToRead,
										LPDWORD				pnNumberOfBytesRead,
										LONGLONG			nOffset)
{
	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return -ERROR_INVALID_HANDLE;

	return openFileData->File->Read(	pBuffer,
										nNumberOfBytesToRead,
										pnNumberOfBytesRead,
										nOffset,
										openFileData->Data);
}


int					PlisgoVFS::Write(	PlisgoFileHandle&	rHandle,
										LPCVOID				pBuffer,
										DWORD				nNumberOfBytesToWrite,
										LPDWORD				pnNumberOfBytesWritten,
										LONGLONG			nOffset)
{
	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return -ERROR_INVALID_HANDLE;

	return openFileData->File->Write(	pBuffer,
										nNumberOfBytesToWrite,
										pnNumberOfBytesWritten,
										nOffset,
										openFileData->Data);
}


int					PlisgoVFS::LockFile(PlisgoFileHandle&	rHandle,
										LONGLONG			nByteOffset,
										LONGLONG			nByteLength)
{
	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return -ERROR_INVALID_HANDLE;

	return openFileData->File->LockFile(	nByteOffset,
											nByteLength,
											openFileData->Data);
}


int					PlisgoVFS::UnlockFile(	PlisgoFileHandle&	rHandle,
											LONGLONG			nByteOffset,
											LONGLONG			nByteLength)
{
	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return -ERROR_INVALID_HANDLE;

	return openFileData->File->UnlockFile(	nByteOffset,
											nByteLength,
											openFileData->Data);
}


int					PlisgoVFS::FlushBuffers(PlisgoFileHandle&	rHandle)
{
	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return -ERROR_INVALID_HANDLE;

	return openFileData->File->FlushBuffers(openFileData->Data);
}


int					PlisgoVFS::SetEndOfFile(PlisgoFileHandle&	rHandle, LONGLONG nEndPos)
{
	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return -ERROR_INVALID_HANDLE;

	return openFileData->File->SetEndOfFile(nEndPos, openFileData->Data);
}


int					PlisgoVFS::Close(PlisgoFileHandle&	rHandle, bool bDeleteOnClose)
{
	if (rHandle == 0)
		return 0; //Already closed

	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return -ERROR_INVALID_HANDLE;

	assert(openFileData.use_count() == 2); //1 for this call, 1 for open handle list

	if (openFileData.use_count() != 2) 
		return 0; //Someone else will close it

	int nError = openFileData->File->Close(openFileData->Data);

	const std::wstring& rsPath = openFileData->sPath;

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

			nError = pFolder->RemoveChild(GetNameFromPath(openFileData->sPath.c_str()));

			if (nError == 0)
			{
				RemoveCached(openFileData->sPath);

				m_MountTree.RemoveBranch(openFileData->sPath);

				if (openFileData->File->GetAsFolder() != NULL)
					RestartCache();
			}
		}	
	}

	boost::unique_lock<boost::shared_mutex>	lock(m_OpenFileMutex);

	if (m_pLog != NULL)
		m_pLog->CloseFile(openFileData->sPath);

	if (openFileData->nFlags & PLISGOVFS_DELETE_ON_CLOSE)
		m_PendingDeletes.erase(rsPath);

	m_OpenFileFactory.FreeOpenFile(rHandle);
	
	rHandle = 0;

	return nError;
}


int					PlisgoVFS::GetDeleteError(PlisgoFileHandle&	rHandle) const
{
	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return -ERROR_INVALID_HANDLE;

	const std::wstring sPath = openFileData->sPath;

	size_t nSlash = sPath.rfind(L'\\');

	nSlash = (nSlash == -1)?0:nSlash;

	IPtrPlisgoFSFile parent = TracePath(sPath.substr(0, nSlash));

	assert(parent.get() != NULL);

	PlisgoFSFolder* pFolder = parent->GetAsFolder();

	assert(pFolder != NULL);

	boost::unique_lock<boost::shared_mutex>	lock(m_OpenFileMutex);

	int nError = pFolder->GetRemoveChildError(GetNameFromPath(openFileData->sPath.c_str()));

	if (nError != 0)
		return nError;

	nError = openFileData->File->GetDeleteError(openFileData->Data);

	if (nError != 0)
		return nError;

	openFileData->nFlags |= PLISGOVFS_DELETE_ON_CLOSE;
	m_PendingDeletes[openFileData->sPath] = true;

	return 0;
}


int					PlisgoVFS::SetFileTimes(	PlisgoFileHandle&	rHandle,
												const FILETIME*		pCreation,
												const FILETIME*		pLastAccess,
												const FILETIME*		pLastWrite)
{
	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return -ERROR_INVALID_HANDLE;

	return openFileData->File->SetFileTimes(pCreation, pLastAccess, pLastWrite, openFileData->Data);
}


int					PlisgoVFS::SetAttributes(	PlisgoFileHandle&	rHandle,
												DWORD				nFileAttributes)
{
	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return -ERROR_INVALID_HANDLE;

	return openFileData->File->SetAttributes(nFileAttributes, openFileData->Data);
}


int					PlisgoVFS::GetHandleInfo(PlisgoFileHandle&	rHandle, LPBY_HANDLE_FILE_INFORMATION pInfo)
{
	OpenFileDataSP openFileData;

	if (!GetOpenFileData(openFileData, rHandle))
		return -ERROR_INVALID_HANDLE;

	return openFileData->File->GetHandleInfo(pInfo, openFileData->Data);
}

	
void				PlisgoVFS::CloseAllOpenFiles()
{
	boost::unique_lock<boost::shared_mutex>	lock(m_OpenFileMutex);

	m_OpenFileFactory.CloseAll(m_pLog);
}


void				PlisgoVFS::AddToCache(const std::wstring& rsLowerPath, IPtrPlisgoFSFile file)
{
	if (file.get() != NULL && file->IsValid())
	{
		boost::unique_lock<boost::shared_mutex> writeLock(m_CacheEntryMutex);

		AddToCacheLocked(rsLowerPath, file, false);
	}
}


void				PlisgoVFS::AddToCacheLocked(const std::wstring& rsLowerPath, IPtrPlisgoFSFile file, bool bKeep)
{
	Cached*& rpCached = m_CacheEntryMap[rsLowerPath];

	if (rpCached != NULL)
		RemoveCached(rpCached);

	rpCached		= m_CacheEntryPool.construct();
	rpCached->file	= file;
	rpCached->sPath	= rsLowerPath;
	
	if (bKeep)
		rpCached->nTime = -1;//From the future, so is never too old!
	else
		GetSystemTimeAsFileTime((FILETIME*)&rpCached->nTime);

	if (m_pOldest == NULL)
		m_pOldest = rpCached;

	if (m_pNewest != NULL)
		m_pNewest->pNext = rpCached;

	rpCached->pPrev = m_pNewest;
	rpCached->pNext = NULL;

	m_pNewest = rpCached;
}


void				PlisgoVFS::RemoveCached(Cached* pCached)
{
	if (pCached == NULL)
		return;

	if (pCached->pNext != NULL)
		pCached->pNext->pPrev = pCached->pPrev;

	if (pCached->pPrev != NULL)
		pCached->pPrev->pNext = pCached->pNext;

	if (pCached == m_pNewest)
		m_pNewest = pCached->pPrev;

	if (pCached == m_pOldest)
		m_pOldest = m_pOldest->pNext;

	m_CacheEntryPool.destroy(pCached);
}

	
void				PlisgoVFS::ClearCache()
{
	boost::unique_lock<boost::shared_mutex> writeLock(m_CacheEntryMutex);

	return RestartCache();
}


void				PlisgoVFS::RestartCache()
{
	//m_CacheEntryMutex should be write-locked

	for(CacheEntryMap::iterator it = m_CacheEntryMap.begin(); it != m_CacheEntryMap.end(); ++it)
		RemoveCached(it->second);

	m_CacheEntryMap.clear();

	MountTree::FullKeyMap fullKeyMap;

	m_MountTree.GetFullKeyMap(fullKeyMap);

	for(MountTree::FullKeyMap::const_iterator it = fullKeyMap.begin(); it != fullKeyMap.end(); ++it)
		AddToCacheLocked(it->first, it->second, true);
}


size_t				PlisgoVFS::ClearOlderThan(int nSeconds)
{
	boost::unique_lock<boost::shared_mutex> writeLock(m_CacheEntryMutex);
	
	ULONG64 nNow;
	
	GetSystemTimeAsFileTime((FILETIME*)&nNow);

	Cached* pEntry = m_pOldest;

	size_t nRemoved = 0;


	while(pEntry != NULL)
	{
		if (pEntry->nTime == -1)
		{
			pEntry = pEntry->pNext;
			continue;
		}

		int nAgeSecs = (int)((nNow - pEntry->nTime) / 10000000); //NTSECOND

		if (nAgeSecs > nSeconds)
		{
			Cached* pNext = pEntry->pNext;

			m_CacheEntryMap.erase(pEntry->sPath);

			RemoveCached(pEntry);

			++nRemoved;

			pEntry = pNext;
		}
		else break;
	}

	return nRemoved;
}