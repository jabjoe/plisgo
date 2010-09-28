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

#pragma once

#include "PlisgoFSFiles.h"
#include "TreeCache.h"


class PlisgoVFSOpenLog
{
public:
	virtual void OpenFileFailed(const std::wstring& rsPath, int nError) = 0;
	virtual void OpenFile(const std::wstring& rsPath) = 0;
	virtual void CloseFile(const std::wstring& rsPath) = 0;
};



class PlisgoVFS
{
public:
	PlisgoVFS(IPtrPlisgoFSFolder root, PlisgoVFSOpenLog* pLog = NULL);

	IPtrPlisgoFSFile			TracePath(LPCWSTR sPath, IPtrPlisgoFSFile* pParent = NULL) const;

	bool						AddMount(LPCWSTR sMount, IPtrPlisgoFSFile Mount);
	bool						RemoveMount(LPCWSTR sMount);
	IPtrPlisgoFSFile			GetMount(LPCWSTR sMount);
	void						GetMounts(std::map<std::wstring,IPtrPlisgoFSFile>& rMounts);


	IPtrPlisgoFSFolder			GetRoot() const			{ return m_Root; }

	int							GetChildren(LPCWSTR sPath, PlisgoFSFolder::ChildNames& rChildren) const;

	typedef ULONG64	PlisgoFileHandle;

	
	

	int							Repath(LPCWSTR sOldPath, LPCWSTR sNewPath, bool bReplaceExisting = false);

	int							GetChildren(PlisgoFileHandle& rHandle, PlisgoFSFolder::ChildNames& rChildren) const;

	void						GetOpenFilePath(PlisgoFileHandle& rHandle, std::wstring& rsPath);

	IPtrPlisgoFSFile			GetParent(PlisgoFileHandle& rHandle) const;
	int							GetChild(IPtrPlisgoFSFile& rChild, PlisgoFileHandle& rHandle, LPCWSTR sChildName) const;

	IPtrPlisgoFSFile			GetPlisgoFSFile(PlisgoFileHandle& rHandle) const;


	int							Open(	PlisgoFileHandle&	rHandle,
										LPCWSTR				sPath,
										DWORD				nDesiredAccess,
										DWORD				nShareMode,
										DWORD				nCreationDisposition,
										DWORD				nFlagsAndAttributes);

	int							Read(	PlisgoFileHandle&	rHandle,
										LPVOID				pBuffer,
										DWORD				nNumberOfBytesToRead,
										LPDWORD				pnNumberOfBytesRead,
										LONGLONG			nOffset);

	int							Write(	PlisgoFileHandle&	rHandle,
										LPCVOID				pBuffer,
										DWORD				nNumberOfBytesToWrite,
										LPDWORD				pnNumberOfBytesWritten,
										LONGLONG			nOffset);

	int							LockFile(	PlisgoFileHandle&	rHandle,
											LONGLONG			nByteOffset,
											LONGLONG			nByteLength);

	int							UnlockFile(	PlisgoFileHandle&	rHandle,
											LONGLONG			nByteOffset,
											LONGLONG			nByteLength);

	int							FlushBuffers(PlisgoFileHandle&	rHandle);

	int							SetEndOfFile(PlisgoFileHandle&	rHandle, LONGLONG nEndPos);

	int							Close(PlisgoFileHandle&	rHandle, bool bDeleteOnClose = false);

	int							GetDeleteError(PlisgoFileHandle&	rHandle) const;

	int							SetFileTimes(	PlisgoFileHandle&	rHandle,
												const FILETIME*		pCreation,
												const FILETIME*		pLastAccess,
												const FILETIME*		pLastWrite);

	int							SetAttributes(	PlisgoFileHandle&	rHandle,
												DWORD				nFileAttributes);

	int							GetHandleInfo(PlisgoFileHandle&	rHandle, LPBY_HANDLE_FILE_INFORMATION pInfo);

	void						CloseAllOpenFiles(); //When the drive dies rather then closes, this lets you clean up.
	void						ClearCache();

protected:
	
	IPtrPlisgoFSFile			TracePath(const std::wstring& rsLowerPath, IPtrPlisgoFSFile* pParent = NULL) const;

	bool						GetCached(const std::wstring& rsPath, IPtrPlisgoFSFile& rFile) const;

	void						RemoveDownstreamMounts(std::wstring sLowerCaseFile);
	void						MoveMounts(std::wstring sOld, std::wstring sNew);

	void						AddToCache(const std::wstring& rsLowerPath, IPtrPlisgoFSFile file);

private:

	void						RestartCache();


	struct OpenFileData
	{
		std::wstring		sPath;
		IPtrPlisgoFSFile	File;
		ULONG64				nData;
		bool				bDeleteOnClose;
		volatile LONG		nUseRef;

		OpenFileData*		pNext;
		OpenFileData*		pPrev;
	};

protected:

	void						DecrementOpenDataRef(OpenFileData* pOpenFileData);

private:

	class OpenFileDataSP
	{
	public:

		OpenFileDataSP(const PlisgoVFS* pPlisgoVFS)	{ m_pPlisgoVFS = (PlisgoVFS*)pPlisgoVFS; m_pData = NULL; }
		~OpenFileDataSP()	{ m_pPlisgoVFS->DecrementOpenDataRef(m_pData); }

		OpenFileData* operator->()	{ return m_pData; }
		OpenFileData* operator*()	{ return m_pData; }

		void	operator = (OpenFileData* pData)
		{
			m_pPlisgoVFS->DecrementOpenDataRef(m_pData);

			m_pData = pData;

			if (m_pData != NULL)
				InterlockedIncrement(&m_pData->nUseRef);
		}

	private:
		OpenFileData*	m_pData;
		PlisgoVFS*		m_pPlisgoVFS;
	};

	bool				GetOpenFileData(OpenFileDataSP& rResult, const PlisgoFileHandle&	rHandle) const;


	IPtrPlisgoFSFolder									m_Root;

	mutable boost::shared_mutex							m_OpenFilePoolMutex;
	boost::object_pool<OpenFileData>					m_OpenFilePool;
	volatile LONG										m_OpenFileNum;
	OpenFileData*										m_pLatestOpen;
	mutable boost::unordered_map<std::wstring, bool>	m_PendingDeletes;

	struct Cached
	{
		IPtrPlisgoFSFile	file;
		ULONG64				nTime;
	};

	typedef std::pair<std::wstring, Cached>																							CacheEntryPair;
	typedef boost::fast_pool_allocator< CacheEntryPair >																			CacheEntryPairPool;
	typedef boost::unordered_map<std::wstring, Cached, boost::hash<std::wstring>, std::equal_to<std::wstring>, CacheEntryPairPool >	CacheEntryMap;

	mutable boost::shared_mutex							m_CacheEntryMutex;
	CacheEntryMap										m_CacheEntryMap;

	typedef TreeCache<IPtrPlisgoFSFile>					MountTree;

	MountTree											m_MountTree;
	PlisgoVFSOpenLog*									m_pLog;
};

typedef boost::shared_ptr<PlisgoVFS>	IPtrPlisgoVFS;