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
	m_nCacheEntryMaxLife = 10000000 * 30; //NTSECOND * 30
	m_pLog = pLog;
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


bool				PlisgoVFS::GetCached(const std::wstring& rsPath, IPtrPlisgoFSFile& rFile) const
{
	boost::upgrade_lock<boost::shared_mutex> readLock(m_CacheEntryMutex);

	CacheEntryMap::const_iterator it = m_CacheEntryMap.find(rsPath);

	if (it == m_CacheEntryMap.end())
		return false;

	const Cached& rCached = it->second;

	rFile = rCached.file;

	ULONG64	nNow;

	GetSystemTimeAsFileTime((FILETIME*)&nNow);

	if ((rCached.nTime < nNow &&  nNow-rCached.nTime > m_nCacheEntryMaxLife) ||		
		(rFile.get() != NULL && !rFile->IsValid()))
	{
		boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock(readLock);

		const_cast<PlisgoVFS*>(this)->m_CacheEntryMap.erase(it);

		return false;
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


int					PlisgoVFS::CreateFolder(LPCWSTR sPath)
{
	if (sPath == NULL)
		return -ERROR_BAD_PATHNAME;

	LPCWSTR sName = GetNameFromPath(sPath);

	std::wstring sPathLowerCase = sPath;

	MakePathHashSafe(sPathLowerCase);

	size_t nSlash = sPathLowerCase.rfind(L"\\");

	if (nSlash == -1)
		nSlash = 0;

	IPtrPlisgoFSFile parent = TracePath(sPathLowerCase.substr(0, nSlash) );

	if (parent.get() == NULL)
		return -ERROR_BAD_PATHNAME;

	PlisgoFSFolder* pFolder = parent->GetAsFolder();

	if (pFolder == NULL)
		return -ERROR_BAD_PATHNAME;

	IPtrPlisgoFSFile child;

	int nError = pFolder->CreateChild(child, sName, FILE_ATTRIBUTE_DIRECTORY);

	if (child.get() != NULL)
		AddToCache(sPathLowerCase, child);

	return nError;
}


class MountSensitiveEachChild : public PlisgoFSFolder::EachChild 
{
public:

	typedef std::map<std::wstring, IPtrPlisgoFSFile>	ChildMountTable;

	MountSensitiveEachChild(PlisgoFSFolder::EachChild&	rCB,
							const ChildMountTable&		rChildMnts) :
																m_CB(rCB),
																m_rChildMnts(rChildMnts)
	{
	}					

	virtual bool Do(LPCWSTR sUnknownCaseName, IPtrPlisgoFSFile file)
	{
		WCHAR sName[MAX_PATH];

		CopyToLower(sName, MAX_PATH, sUnknownCaseName);
		
		const ChildMountTable::const_iterator it = m_rChildMnts.find(sName);

		if (it != m_rChildMnts.end())
			return m_CB.Do(sUnknownCaseName, it->second);

		return m_CB.Do(sUnknownCaseName, file);
	}

	PlisgoFSFolder::EachChild&	m_CB;
	const ChildMountTable&		m_rChildMnts;
};


int					PlisgoVFS::ForEachChild(LPCWSTR sPath, PlisgoFSFolder::EachChild& rCB) const
{
	std::wstring sPathLowerCase = sPath;

	MakePathHashSafe(sPathLowerCase);

	IPtrPlisgoFSFile	file = TracePath(sPathLowerCase);

	if (file.get() == NULL)
		return -ERROR_BAD_PATHNAME;

	return ForEachChild(file->GetAsFolder(), sPathLowerCase, rCB);
}


int					PlisgoVFS::ForEachChild(PlisgoFSFolder* pFolder, const std::wstring& rsLowerCasePath, PlisgoFSFolder::EachChild& rCB) const
{
	if (pFolder == NULL)
		return -ERROR_ACCESS_DENIED;

	MountTree::SubKeyMap mntChildren;

	m_MountTree.GetChildMap(rsLowerCasePath, mntChildren);

	if (mntChildren.size())
	{
		MountSensitiveEachChild cb(rCB, mntChildren);

		pFolder->ForEachChild(cb);

		return 0;
	}

	pFolder->ForEachChild(rCB);

	return 0;
}


int					PlisgoVFS::ForEachChild(PlisgoFileHandle&	rHandle, PlisgoFSFolder::EachChild& rCB) const
{
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return -ERROR_INVALID_HANDLE;

	return ForEachChild(pOpenFileData->File->GetAsFolder(), pOpenFileData->sPath, rCB);
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

	if (file.get() == NULL)
	{
		if (nCreationDisposition == OPEN_EXISTING ||
			nCreationDisposition == TRUNCATE_EXISTING)
			return -ERROR_FILE_NOT_FOUND;

		size_t nSlash = sPathLowerCase.rfind(L'\\');

		nSlash = (nSlash == -1)?0:nSlash;

		IPtrPlisgoFSFile parent = TracePath(sPathLowerCase.substr(0, nSlash));

		if (parent.get() == NULL)
			return -ERROR_PATH_NOT_FOUND;
		
		PlisgoFSFolder* pFolder = parent->GetAsFolder();

		if (pFolder == NULL)
			return -ERROR_PATH_NOT_FOUND; //wtf, silly user

		nError = pFolder->CreateChild(file, GetNameFromPath(sPath), nFlagsAndAttributes);

		if (file.get() != NULL)
		{
			AddToCache(sPathLowerCase, file);

			nError = file->Open(nDesiredAccess, nShareMode, OPEN_EXISTING, nFlagsAndAttributes, &nOpenInstaceData);
		}
		else
		{
			assert(nError != 0);
		}
	}
	else nError = file->Open(nDesiredAccess, nShareMode, nCreationDisposition, nFlagsAndAttributes, &nOpenInstaceData);

	if (nError != 0)
		return nError;

	assert(file.get() != NULL);

	boost::unique_lock<boost::shared_mutex>	lock(m_OpenFilePoolMutex);
	
	OpenFileData* pOpenFileData = m_OpenFilePool.construct();

	if (pOpenFileData == NULL)
	{
		file->Close(&nOpenInstaceData);

		return -ERROR_TOO_MANY_OPEN_FILES;
	}

	InterlockedIncrement(&m_OpenFileNum);

	pOpenFileData->File = file;

	pOpenFileData->sPath = sPathLowerCase;
	pOpenFileData->nData = nOpenInstaceData;

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
	OpenFileData* pOpenFileData = GetOpenFileData(rHandle);

	if (pOpenFileData == NULL)
		return -ERROR_INVALID_HANDLE;

	int nError = pOpenFileData->File->Close(&pOpenFileData->nData);

	if (nError != 0)
		return nError;

	if (bDeleteOnClose)
	{
		const std::wstring& rsPath = pOpenFileData->sPath;

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
			else return nError;
		}	
	}

	boost::unique_lock<boost::shared_mutex>	lock(m_OpenFilePoolMutex);

	if (m_pLog != NULL)
		m_pLog->CloseFile(pOpenFileData->sPath);

	m_OpenFilePool.destroy(pOpenFileData);

	InterlockedDecrement(&m_OpenFileNum);

	return 0;
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

	int nError = pFolder->GetRemoveChildError(GetNameFromPath(pOpenFileData->sPath));

	if (nError != 0)
		return nError;

	return pOpenFileData->File->GetDeleteError(&pOpenFileData->nData);
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
