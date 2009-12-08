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

#include "PlisgoFSLib.h"

class PlisgoFSFolder;

class PlisgoFSFile
{
public:
	virtual ~PlisgoFSFile()	{}

	virtual DWORD				GetAttributes() const		{ return FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY; }

	virtual bool				GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const
	{
		GetSystemTimeAsFileTime(&rLastWrite);

		rCreation = rLastAccess = rLastWrite;
		return true;
	}

	virtual LONGLONG			GetSize() const = 0;

	virtual int					Open(	DWORD		nDesiredAccess,
										DWORD		nShareMode,
										DWORD		nCreationDisposition,
										DWORD		nFlagsAndAttributes,
										ULONGLONG*	pInstanceData);

	virtual int					Read(	LPVOID		pBuffer,
										DWORD		nNumberOfBytesToRead,
										LPDWORD		pnNumberOfBytesRead,
										LONGLONG	nOffset,
										ULONGLONG*	pInstanceData);

	virtual int					Write(	LPCVOID		pBuffer,
										DWORD		nNumberOfBytesToWrite,
										LPDWORD		pnNumberOfBytesWritten,
										LONGLONG	nOffset,
										ULONGLONG*	pInstanceData);

	virtual int					LockFile(	LONGLONG	nByteOffset,
											LONGLONG	nByteLength,
											ULONGLONG*	pInstanceData);

	virtual int					UnlockFile(	LONGLONG	nByteOffset,
											LONGLONG	nByteLength,
											ULONGLONG*	pInstanceData);

	virtual int					FlushBuffers(ULONGLONG* pInstanceData);

	virtual int					SetEndOfFile(LONGLONG nEndPos, ULONGLONG* pInstanceData);

	virtual int					Close(ULONGLONG* pInstanceData);

	virtual int					GetDeleteError(ULONGLONG* pInstanceData) const	{ return -ERROR_ACCESS_DENIED; }

	virtual int					SetFileTimes(	const FILETIME* pCreation,
												const FILETIME* pLastAccess,
												const FILETIME* pLastWrite,
												ULONGLONG*		pInstanceData);

	virtual int					SetAttributes(	DWORD		nFileAttributes,
												ULONGLONG*	pInstanceData);

	virtual int					GetHandleInfo(LPBY_HANDLE_FILE_INFORMATION pInfo, ULONGLONG* pInstanceData);

	virtual PlisgoFSFolder*		GetAsFolder() const							{ return NULL; }
	template<typename T>
	void						GetFileInfo(T* pFileInfo) const;
};


template<typename T>
inline void		PlisgoFSFile::GetFileInfo(T* pFileInfo) const
{
	assert(pFileInfo != NULL);

	ZeroMemory(pFileInfo, sizeof(T));

	pFileInfo->dwFileAttributes = GetAttributes();

	if (!GetFileTimes(pFileInfo->ftCreationTime, pFileInfo->ftLastAccessTime, pFileInfo->ftLastWriteTime))
	{
		GetSystemTimeAsFileTime(&pFileInfo->ftCreationTime);
		pFileInfo->ftLastAccessTime = pFileInfo->ftLastWriteTime = pFileInfo->ftCreationTime;
	}

	ULARGE_INTEGER nSize;

	nSize.QuadPart = GetSize();

	pFileInfo->nFileSizeHigh	= nSize.HighPart;
	pFileInfo->nFileSizeLow		= nSize.LowPart;
}



typedef boost::shared_ptr<PlisgoFSFile>	IPtrPlisgoFSFile;



class PlisgoFSFolder : public PlisgoFSFile
{
public:
	virtual DWORD				GetAttributes() const						{ return FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY; }
	virtual LONGLONG			GetSize() const								{ return 0; }

	class EachChild
	{
	public:
		virtual bool Do(LPCWSTR sName, IPtrPlisgoFSFile file) = 0;
	};

	virtual int					Open(	DWORD		nDesiredAccess,
										DWORD		nShareMode,
										DWORD		nCreationDisposition,
										DWORD		nFlagsAndAttributes,
										ULONGLONG*	pInstanceData);

	virtual bool				ForEachChild(EachChild& rEachChild) const = 0;
	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sName) const = 0;

	virtual int					AddChild(LPCWSTR sName, IPtrPlisgoFSFile file)	{ return -ERROR_ACCESS_DENIED; }

	virtual int					CreateChild(IPtrPlisgoFSFile& rChild, LPCWSTR sName, DWORD nAttr)	{ return -ERROR_ACCESS_DENIED; }

	virtual int					Repath(LPCWSTR sOldName, LPCWSTR sNewName, bool bReplaceExisting, PlisgoFSFolder* pNewParent);

	virtual int					GetRemoveChildError(LPCWSTR sName) const		{ return -ERROR_ACCESS_DENIED; }
	virtual int					RemoveChild(LPCWSTR sName)						{ return -ERROR_ACCESS_DENIED; }

	PlisgoFSFolder*				GetAsFolder() const								{ return const_cast<PlisgoFSFolder*>(this); }
};

typedef boost::shared_ptr<PlisgoFSFolder>	IPtrPlisgoFSFolder;




class PlisgoFSFileList
{
public:

	void				AddFile(LPCWSTR sName, IPtrPlisgoFSFile file);
	void				RemoveFile(LPCWSTR sName);
	bool				ForEachFile(PlisgoFSFolder::EachChild& rEachFile) const;
	IPtrPlisgoFSFile	GetFile(LPCWSTR sName) const;
	IPtrPlisgoFSFile	GetFile(UINT nIndex) const;
	UINT				GetLength() const;

private:
	mutable boost::shared_mutex					m_Mutex;
	
	std::map<std::wstring, IPtrPlisgoFSFile >	m_files;
};



class PlisgoFSStorageFolder : public PlisgoFSFolder
{
public:


	virtual bool				ForEachChild(PlisgoFSFolder::EachChild& rEachChild) const
	{
		return m_childList.ForEachFile(rEachChild);
	}

	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sName) const
	{
		return m_childList.GetFile(sName);
	}

	virtual int					AddChild(LPCWSTR sName, IPtrPlisgoFSFile file)
	{
		m_childList.AddFile(sName, file);

		return 0;
	}

	virtual int					CreateChild(IPtrPlisgoFSFile& rChild, LPCWSTR sName, DWORD nAttr);

	virtual int					GetRemoveChildError(LPCWSTR sName) const		{ return 0; }
	virtual int					RemoveChild(LPCWSTR sName)						{ m_childList.RemoveFile(sName); return 0; }

protected:
	PlisgoFSFileList		m_childList;
};





class PlisgoFSDataFile : public PlisgoFSFile
{
public:
	PlisgoFSDataFile(	BYTE*				pData,
						size_t				nDataSize,
						bool				bReadOnly = true,
						bool				bCopy = false);

	PlisgoFSDataFile();

	~PlisgoFSDataFile();

	virtual DWORD			GetAttributes() const							{ return FILE_ATTRIBUTE_HIDDEN|((m_bReadOnly)?FILE_ATTRIBUTE_READONLY:0); }
	virtual LONGLONG		GetSize() const									{ return m_nDataUsedSize; }
	const BYTE*				GetData() const									{ return m_pData; }

	virtual bool			GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const;
	
	virtual int				Open(	DWORD		nDesiredAccess,
									DWORD		nShareMode,
									DWORD		nCreationDisposition,
									DWORD		nFlagsAndAttributes,
									ULONGLONG*	pInstanceData);

	virtual int				Read(	LPVOID		pBuffer,
									DWORD		nNumberOfBytesToRead,
									LPDWORD		pnNumberOfBytesRead,
									LONGLONG	nOffset,
									ULONGLONG*	pInstanceData);

	virtual int				Write(	LPCVOID		pBuffer,
									DWORD		nNumberOfBytesToWrite,
									LPDWORD		pnNumberOfBytesWritten,
									LONGLONG	nOffset,
									ULONGLONG*	pInstanceData);

	virtual int				SetEndOfFile(LONGLONG nEndPos, ULONGLONG* pInstanceData);

	virtual int				Close(ULONGLONG* pInstanceData);

	bool					IsReadOnly() const							{ return m_bReadOnly; }
	void					SetReadOnly(bool bReadOnly);

private:

	void					GrowToSize(size_t nDataSize);

	void					CreateOwnCopy(BYTE* pData, size_t nDataSize);

	BYTE*						m_pData;
	ULONG						m_nDataUsedSize;
	ULONG						m_nDataSize;
	bool						m_bOwnMemory;
	bool						m_bReadOnly;
	bool						m_bWriteOpen;
	boost::shared_mutex			m_Mutex;

	FILETIME					m_CreatedTime;
	FILETIME					m_LastWrite;

};



class PlisgoFSStringFile : public  PlisgoFSFile
{
public:
	PlisgoFSStringFile()												{ m_bReadOnly = m_bWriteOpen = false; }

	PlisgoFSStringFile(	const std::wstring& sData, 
						bool				bReadOnly = false);

	PlisgoFSStringFile(	const std::string& sData, 
						bool bReadOnly = false);

	void				SetString(const std::wstring& rsData);
	void				AddString(const std::wstring& rsData);
	void				GetWideString(std::wstring& rResult);

	void				SetString(const std::string& rsData);
	void				AddString(const std::string& rsData);
	void				GetString(std::string& rsResult);

	virtual DWORD		GetAttributes() const							{ return FILE_ATTRIBUTE_HIDDEN|((m_bReadOnly)?FILE_ATTRIBUTE_READONLY:0); }
	virtual LONGLONG	GetSize() const;

	virtual int			Open(	DWORD		nDesiredAccess,
								DWORD		nShareMode,
								DWORD		nCreationDisposition,
								DWORD		nFlagsAndAttributes,
								ULONGLONG*	pInstanceData);

	virtual int			Read(	LPVOID		pBuffer,
								DWORD		nNumberOfBytesToRead,
								LPDWORD		pnNumberOfBytesRead,
								LONGLONG	nOffset,
								ULONGLONG*	pInstanceData);

	virtual int			Write(	LPCVOID		pBuffer,
								DWORD		nNumberOfBytesToWrite,
								LPDWORD		pnNumberOfBytesWritten,
								LONGLONG	nOffset,
								ULONGLONG*	pInstanceData);

	virtual int			LockFile(	LONGLONG	nByteOffset,
									LONGLONG	nByteLength,
									ULONGLONG*	pInstanceData);

	virtual int			UnlockFile(	LONGLONG	nByteOffset,
									LONGLONG	nByteLength,
									ULONGLONG*	pInstanceData);

	virtual int			SetEndOfFile(LONGLONG nEndPos, ULONGLONG* pInstanceData);

	virtual int			Close(ULONGLONG* pInstanceData);

	bool				IsReadOnly() const							{ return m_bReadOnly; }
	void				SetReadOnly(bool bReadOnly);

private:
	bool							m_bReadOnly;
	bool							m_bWriteOpen;

	std::string						m_sData;
	mutable boost::shared_mutex		m_Mutex;
};




class PlisgoFSRedirectionFile : public PlisgoFSFile
{
public:

	PlisgoFSRedirectionFile(const std::wstring& rsRealFile); 


	virtual DWORD			GetAttributes() const;
	virtual LONGLONG		GetSize() const;

	virtual bool			GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const;


	virtual int				Open(	DWORD		nDesiredAccess,
									DWORD		nShareMode,
									DWORD		nCreationDisposition,
									DWORD		nFlagsAndAttributes,
									ULONGLONG*	pInstanceData);

	virtual int				Read(	LPVOID		pBuffer,
									DWORD		nNumberOfBytesToRead,
									LPDWORD		pnNumberOfBytesRead,
									LONGLONG	nOffset,
									ULONGLONG*	pInstanceData);

	virtual int				Write(	LPCVOID		pBuffer,
									DWORD		nNumberOfBytesToWrite,
									LPDWORD		pnNumberOfBytesWritten,
									LONGLONG	nOffset,
									ULONGLONG*	pInstanceData);

	virtual int				LockFile(	LONGLONG	nByteOffset,
										LONGLONG	nByteLength,
										ULONGLONG*	pInstanceData);

	virtual int				UnlockFile(	LONGLONG	nByteOffset,
										LONGLONG	nByteLength,
										ULONGLONG*	pInstanceData);

	virtual int				GetHandleInfo(LPBY_HANDLE_FILE_INFORMATION pInfo, ULONGLONG* pInstanceData);

	virtual int				FlushBuffers(ULONGLONG* pInstanceData);

	virtual int				SetEndOfFile(LONGLONG nEndPos, ULONGLONG* pInstanceData);

	virtual int				GetDeleteError(ULONGLONG* pInstanceData) const	{ return 0; }

	virtual int				Close(ULONGLONG* pInstanceData);

	virtual int				SetFileTimes(	const FILETIME* pCreation,
											const FILETIME* pLastAccess,
											const FILETIME*	pLastWrite,
											ULONGLONG*		pInstanceData);

	virtual int				SetAttributes(DWORD	nFileAttributes, ULONGLONG* pInstanceData);

	const std::wstring&		GetRealPath() const		{ return m_sRealFile; }

private:

	std::wstring	m_sRealFile;
};



class PlisgoFSRedirectionFolder : public PlisgoFSFolder
{
public:

	PlisgoFSRedirectionFolder(const std::wstring& rsRealFolder); 

	virtual DWORD				GetAttributes() const;

	virtual bool				GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const;

	virtual int					SetFileTimes(	const FILETIME* pCreation,
												const FILETIME* pLastAccess,
												const FILETIME*	pLastWrite,
												ULONGLONG*		pInstanceData);

	virtual int					SetAttributes(DWORD	nFileAttributes, ULONGLONG* pInstanceData);

	virtual bool				ForEachChild(PlisgoFSFolder::EachChild& rEachChild) const;

	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sName) const;

	virtual int					AddChild(LPCWSTR sName, IPtrPlisgoFSFile file);
	virtual int					CreateChild(IPtrPlisgoFSFile& rChild, LPCWSTR sName, DWORD nAttr);

	virtual int					Repath(LPCWSTR sOldName, LPCWSTR sNewName, bool bReplaceExisting, PlisgoFSFolder* pNewParent);

	const std::wstring&			GetRealPath() const		{ return m_sRealPath; }

	virtual int					GetDeleteError(ULONGLONG* pInstanceData) const;

	virtual int					GetRemoveChildError(LPCWSTR sName) const;

	virtual int					RemoveChild(LPCWSTR sName);

private:

	IPtrPlisgoFSFile			GetChild(	WIN32_FIND_DATAW& rFind) const;

	std::wstring				m_sRealPath;
};



class PlisgoVFS
{
	friend class MountSensitiveEachChild;
public:
	PlisgoVFS(IPtrPlisgoFSFolder root)
	{
		assert(root.get() != NULL);
		m_Root = root;
	}

	IPtrPlisgoFSFile			TracePath(LPCWSTR sPath, IPtrPlisgoFSFile* pParent = NULL) const;

	bool						AddMount(LPCWSTR sMount, IPtrPlisgoFSFile Mount);

	int							Repath(LPCWSTR sOldPath, LPCWSTR sNewPath, bool bReplaceExisting = false);

	IPtrPlisgoFSFolder			GetRoot() const			{ return m_Root; }


	typedef ULONG64	PlisgoFileHandle;


	int							CreateFolder(LPCWSTR sPath);
	int							ForEachChild(PlisgoFileHandle&	rHandle, PlisgoFSFolder::EachChild& rCB) const;


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


protected:
	
	IPtrPlisgoFSFile			TracePath(const std::wstring& rsLowerPath) const;

	bool						GetCached(const std::wstring& rsPath, IPtrPlisgoFSFile& rFile) const;

	void						RemoveDownstreamMounts(std::wstring sLowerCaseFile);
	void						MoveMounts(std::wstring sOld, std::wstring sNew);

	IPtrPlisgoFSFile			GetChildMount(std::wstring& rsParentPath, LPCWSTR sName) const;

	void						AddToCache(const std::wstring& rsLowerPath, IPtrPlisgoFSFile file);

	//typedef boost::unordered_map<std::wstring, IPtrPlisgoFSFile>	ChildMountTable;
	typedef std::map<std::wstring, IPtrPlisgoFSFile>	ChildMountTable;

private:


	//typedef boost::unordered_map<std::wstring, ChildMountTable>		ParentMountTable;
	typedef std::map<std::wstring, ChildMountTable>		ParentMountTable;

	struct OpenFileData
	{
		std::wstring		sPath;
		IPtrPlisgoFSFile	File;
		ULONG64				nData;
	};

	OpenFileData*				GetOpenFileData(PlisgoFileHandle&	rHandle) const;


	IPtrPlisgoFSFolder						m_Root;

	mutable boost::shared_mutex				m_MountsMutex;
	ParentMountTable						m_ParentMounts;

	mutable boost::shared_mutex				m_OpenFilePoolMutex;
	boost::object_pool<OpenFileData>		m_OpenFilePool;


	struct Cached
	{
		IPtrPlisgoFSFile	file;
		ULONG64				nTime;
	};

	typedef std::pair<std::wstring, Cached>																							CacheEntryPair;
	typedef boost::fast_pool_allocator< CacheEntryPair >																			CacheEntryPairPool;
	typedef boost::unordered_map<std::wstring, Cached, boost::hash<std::wstring>, std::equal_to<std::wstring>, CacheEntryPairPool >	CacheEntryMap;

	mutable boost::shared_mutex				m_CacheEntryMutex;
	CacheEntryMap							m_CacheEntryMap;

};

typedef boost::shared_ptr<PlisgoVFS>	IPtrPlisgoVFS;



inline IPtrPlisgoFSFile	GetPlisgoDesktopIniFile()
{
	std::string sData = "[.ShellClassInfo]\r\nCLSID={ADA19F85-EEB6-46F2-B8B2-2BD977934A79}\r\n";

	return IPtrPlisgoFSFile(new PlisgoFSStringFile(sData, true));
}

