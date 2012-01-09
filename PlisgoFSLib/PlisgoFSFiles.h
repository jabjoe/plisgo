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


class PlisgoFSData
{
public:
	virtual ~PlisgoFSData() {}
};

typedef boost::shared_ptr<PlisgoFSData>	IPtrPlisgoFSData;


class PlisgoFSFile
{
public:
	virtual ~PlisgoFSFile()	{}

	virtual bool				IsValid() const				{ return true; }

	virtual DWORD				GetAttributes() const		{ return FILE_ATTRIBUTE_READONLY; }

	virtual bool				GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const
	{
		GetSystemTimeAsFileTime(&rLastWrite);

		rCreation = rLastAccess = rLastWrite;
		return true;
	}

	virtual LONGLONG			GetSize() const = 0;

	virtual int					Open(	DWORD				nDesiredAccess,
										DWORD				nShareMode,
										DWORD				nCreationDisposition,
										DWORD				nFlagsAndAttributes,
										IPtrPlisgoFSData&	rData);

	virtual int					Read(	LPVOID				pBuffer,
										DWORD				nNumberOfBytesToRead,
										LPDWORD				pnNumberOfBytesRead,
										LONGLONG			nOffset,
										IPtrPlisgoFSData&	rData);

	virtual int					Write(	LPCVOID				pBuffer,
										DWORD				nNumberOfBytesToWrite,
										LPDWORD				pnNumberOfBytesWritten,
										LONGLONG			nOffset,
										IPtrPlisgoFSData&	rData);

	virtual int					LockFile(	LONGLONG			nByteOffset,
											LONGLONG			nByteLength,
											IPtrPlisgoFSData&	rData);

	virtual int					UnlockFile(	LONGLONG			nByteOffset,
											LONGLONG			nByteLength,
											IPtrPlisgoFSData&	rData);

	virtual int					FlushBuffers(IPtrPlisgoFSData&	rData);

	virtual int					SetEndOfFile(LONGLONG nEndPos, IPtrPlisgoFSData&	rData);

	virtual int					Close(IPtrPlisgoFSData&	rData);

	virtual int					GetDeleteError(IPtrPlisgoFSData& /*rData*/) const	{ return -ERROR_ACCESS_DENIED; }

	virtual int					SetFileTimes(	const FILETIME*		pCreation,
												const FILETIME*		pLastAccess,
												const FILETIME*		pLastWrite,
												IPtrPlisgoFSData&	rData);

	virtual int					SetAttributes(	DWORD		nFileAttributes,
												IPtrPlisgoFSData&	rData);

	virtual int					GetHandleInfo(LPBY_HANDLE_FILE_INFORMATION pInfo, IPtrPlisgoFSData&	rData);

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
	virtual DWORD				GetAttributes() const						{ return FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_READONLY; }
	virtual LONGLONG			GetSize() const								{ return 0; }

	virtual int					Open(	DWORD				nDesiredAccess,
										DWORD				nShareMode,
										DWORD				nCreationDisposition,
										DWORD				nFlagsAndAttributes,
										IPtrPlisgoFSData&	rData);

	typedef std::vector<std::wstring>	ChildNames;

	virtual int					GetChildren(ChildNames& rChildren) const = 0;
	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sName) const = 0;

	virtual int					AddChild(LPCWSTR /*sName*/, IPtrPlisgoFSFile /*file*/)	{ return -ERROR_ACCESS_DENIED; }

	virtual int					CreateChild(IPtrPlisgoFSFile& /*rChild*/, LPCWSTR /*sName*/, DWORD /*nDesiredAccess*/, DWORD /*nShareMode*/, DWORD /*nAttr*/, IPtrPlisgoFSData&	/*rData*/)	{ return -ERROR_ACCESS_DENIED; }

	virtual int					Repath(LPCWSTR sOldName, LPCWSTR sNewName, bool bReplaceExisting, PlisgoFSFolder* pNewParent);

	virtual int					GetRemoveChildError(LPCWSTR /*sName*/) const	{ return -ERROR_ACCESS_DENIED; }
	virtual int					RemoveChild(LPCWSTR /*sName*/)					{ return -ERROR_ACCESS_DENIED; }

	PlisgoFSFolder*				GetAsFolder() const								{ return const_cast<PlisgoFSFolder*>(this); }
};

typedef boost::shared_ptr<PlisgoFSFolder>	IPtrPlisgoFSFolder;



class PlisgoFSFileMap : public WStringIMap<IPtrPlisgoFSFile>
{
public:
	
	int					GetFileNames(PlisgoFSFolder::ChildNames& rFileNames) const;
	IPtrPlisgoFSFile	GetFile(LPCWSTR sName) const;
};

typedef std::vector<IPtrPlisgoFSFile>											PlisgoFSFileArray;


class PlisgoFSFileList
{
public:

	void				AddFile(LPCWSTR sName, IPtrPlisgoFSFile file);
	void				RemoveFile(LPCWSTR sName);
	int					GetFileNames(PlisgoFSFolder::ChildNames& rFileNames) const;
	IPtrPlisgoFSFile	GetFile(LPCWSTR sName) const;
	UINT				GetLength() const;
	void				Clear();

	void				GetCopy(PlisgoFSFileMap& rCpy);

private:
	
	mutable boost::shared_mutex		m_Mutex;
	PlisgoFSFileMap					m_Files;
};



class PlisgoFSStorageFolder : public PlisgoFSFolder
{
public:


	virtual int					GetChildren(ChildNames& rChildren) const
	{
		return m_childList.GetFileNames(rChildren);
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

	virtual int					CreateChild(IPtrPlisgoFSFile& rChild, LPCWSTR sName, DWORD nDesiredAccess, DWORD nShareMode, DWORD nAttr, IPtrPlisgoFSData&	rData);

	virtual int					GetRemoveChildError(LPCWSTR /*sName*/) const		{ return 0; }
	virtual int					RemoveChild(LPCWSTR sName)						{ m_childList.RemoveFile(sName); return 0; }

protected:
	PlisgoFSFileList		m_childList;
};


class PlisgoFSReadOnlyStorageFolder : public PlisgoFSFolder
{
public:

	PlisgoFSReadOnlyStorageFolder(const PlisgoFSFileMap& rFileMap);

	virtual int 				GetChildren(ChildNames& rChildren) const;
	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sName) const;

	virtual int					AddChild(LPCWSTR , IPtrPlisgoFSFile )													{ return -ERROR_ACCESS_DENIED; }
	virtual int					CreateChild(IPtrPlisgoFSFile& , LPCWSTR , DWORD , DWORD , DWORD , IPtrPlisgoFSData&	)	{ return -ERROR_ACCESS_DENIED; }
	virtual int					GetRemoveChildError(LPCWSTR ) const														{ return -ERROR_ACCESS_DENIED; }
	virtual int					RemoveChild(LPCWSTR )																	{ return -ERROR_ACCESS_DENIED; }

private:

	PlisgoFSFileMap		m_Files;
};



class PlisgoFSUserData : public PlisgoFSData
{
	friend class PlisgoFSUserDataFile;
public:
	PlisgoFSUserData() { m_nPrivate = 0; }
protected:
	ULONG64		m_nPrivate;
};

typedef boost::shared_ptr<PlisgoFSUserData> IPtrPlisgoFSUserData;

class PlisgoFSUserDataFile : public PlisgoFSFile
{
public:
	virtual IPtrPlisgoFSUserData	CreateData();
protected:
	static ULONG64&					GetPrivate(IPtrPlisgoFSData& rData);
};


class PlisgoFSDataFileReadOnly : public PlisgoFSUserDataFile
{
public:
	PlisgoFSDataFileReadOnly(	void*	pData,
								size_t	nDataSize,
								bool	bOwnMemory = false,
								bool	bFreeNotDelete = false);

	~PlisgoFSDataFileReadOnly();

	virtual DWORD			GetAttributes() const							{ return FILE_ATTRIBUTE_READONLY; }
	virtual LONGLONG		GetSize() const									{ return m_nDataSize; }
	const BYTE*				GetData() const									{ return m_pData; }

	virtual bool			GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const
	{
		rCreation = rLastAccess = rLastWrite = m_Time;
		return true;
	}

	virtual int				Open(	DWORD				nDesiredAccess,
									DWORD				nShareMode,
									DWORD				nCreationDisposition,
									DWORD				nFlagsAndAttributes,
									IPtrPlisgoFSData&	rData);

	virtual int				Read(	LPVOID				pBuffer,
									DWORD				nNumberOfBytesToRead,
									LPDWORD				pnNumberOfBytesRead,
									LONGLONG			nOffset,
									IPtrPlisgoFSData&	rData);

private:
	BYTE*			m_pData;
	size_t			m_nDataSize;
	bool			m_bOwnMemory;
	bool			m_bFreeNotDelete;
	FILETIME		m_Time;
};


class PlisgoFSDataFile : public PlisgoFSUserDataFile
{
public:
	PlisgoFSDataFile(	BYTE*				pData,
						size_t				nDataSize,
						bool				bReadOnly = true,
						bool				bCopy = false);

	PlisgoFSDataFile();
	~PlisgoFSDataFile();

	virtual IPtrPlisgoFSUserData	CreateData();


	bool					IsValid() const									{ return !m_bVolatile; }

	bool					SetVolatile(bool bVolatile)						{ m_bVolatile = bVolatile; }

	virtual DWORD			GetAttributes() const							{ return ((m_bReadOnly)?FILE_ATTRIBUTE_READONLY:0); }
	virtual LONGLONG		GetSize() const									{ return m_nDataUsedSize; }
	const BYTE*				GetData() const									{ return m_pData; }

	virtual bool			GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const;
	
	virtual int				Open(	DWORD				nDesiredAccess,
									DWORD				nShareMode,
									DWORD				nCreationDisposition,
									DWORD				nFlagsAndAttributes,
									IPtrPlisgoFSData&	rData);

	virtual int				Read(	LPVOID				pBuffer,
									DWORD				nNumberOfBytesToRead,
									LPDWORD				pnNumberOfBytesRead,
									LONGLONG			nOffset,
									IPtrPlisgoFSData&	rData);

	virtual int				Write(	LPCVOID				pBuffer,
									DWORD				nNumberOfBytesToWrite,
									LPDWORD				pnNumberOfBytesWritten,
									LONGLONG			nOffset,
									IPtrPlisgoFSData&	rData);

	virtual int				SetEndOfFile(LONGLONG nEndPos, IPtrPlisgoFSData& rData);

	virtual int				Close(IPtrPlisgoFSData& rData);

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
	bool						m_bVolatile;

	FILETIME					m_CreatedTime;
	FILETIME					m_LastWrite;

};


class PlisgoFSStringReadOnly : public PlisgoFSUserDataFile
{
public:
	PlisgoFSStringReadOnly();
	PlisgoFSStringReadOnly(	const std::wstring& sData);
	PlisgoFSStringReadOnly(	const std::string& sData);

	virtual IPtrPlisgoFSUserData	CreateData();

	bool				IsValid() const									{ return !m_bVolatile; }
	void				SetVolatile(bool bVolatile)						{ m_bVolatile = bVolatile; }

	virtual void		GetWideString(std::wstring& rResult)			{ ToWide(rResult, m_sData); }
	virtual void		GetString(std::string& rsResult)				{ rsResult = m_sData; }

	virtual DWORD		GetAttributes() const							{ return FILE_ATTRIBUTE_READONLY; }
	virtual LONGLONG	GetSize() const									{ return m_sData.size(); }

	virtual int			Open(	DWORD				nDesiredAccess,
								DWORD				nShareMode,
								DWORD				nCreationDisposition,
								DWORD				nFlagsAndAttributes,
								IPtrPlisgoFSData&	rData);

	virtual int			Read(	LPVOID				pBuffer,
								DWORD				nNumberOfBytesToRead,
								LPDWORD				pnNumberOfBytesRead,
								LONGLONG			nOffset,
								IPtrPlisgoFSData&	rData);

	virtual int			LockFile(	LONGLONG, LONGLONG ,IPtrPlisgoFSData&	)	{ return 0; }
	virtual int			UnlockFile(	LONGLONG, LONGLONG ,IPtrPlisgoFSData&	)	{ return 0; }

	virtual int			Close(IPtrPlisgoFSData&	)								{ return 0; }

	virtual bool		GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const
	{
		rCreation = rLastAccess = rLastWrite = (FILETIME&)m_nTime;

		return true;
	}
	
protected:
	volatile bool					m_bVolatile;
	std::string						m_sData;
	ULONG64							m_nTime;
};


class PlisgoFSStringHiddenReadOnly : public PlisgoFSStringReadOnly
{
public:
	PlisgoFSStringHiddenReadOnly() {}
	PlisgoFSStringHiddenReadOnly(const std::wstring& sData) : PlisgoFSStringReadOnly(sData) {}
	PlisgoFSStringHiddenReadOnly(const std::string& sData) : PlisgoFSStringReadOnly(sData) {}

	virtual DWORD		GetAttributes() const	{ return FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY; }
};


class PlisgoFSStringFile : public PlisgoFSStringReadOnly
{
public:
	PlisgoFSStringFile();
	PlisgoFSStringFile(	const std::wstring& sData);
	PlisgoFSStringFile(	const std::string& sData);

	void				SetString(const std::wstring& rsData);
	void				AddString(const std::wstring& rsData);

	void				SetString(const std::string& rsData);
	void				AddString(const std::string& rsData);

	virtual void		GetWideString(std::wstring& rResult);
	virtual void		GetString(std::string& rsResult);

	virtual DWORD		GetAttributes() const							{ return ((m_bReadOnly)?FILE_ATTRIBUTE_READONLY:0); }
	virtual LONGLONG	GetSize() const;

	virtual int			Open(	DWORD				nDesiredAccess,
								DWORD				nShareMode,
								DWORD				nCreationDisposition,
								DWORD				nFlagsAndAttributes,
								IPtrPlisgoFSData&	rData);

	virtual int			Read(	LPVOID				pBuffer,
								DWORD				nNumberOfBytesToRead,
								LPDWORD				pnNumberOfBytesRead,
								LONGLONG			nOffset,
								IPtrPlisgoFSData&	rData);

	virtual int			Write(	LPCVOID				pBuffer,
								DWORD				nNumberOfBytesToWrite,
								LPDWORD				pnNumberOfBytesWritten,
								LONGLONG			nOffset,
								IPtrPlisgoFSData&	rData);

	virtual int			LockFile(	LONGLONG			nByteOffset,
									LONGLONG			nByteLength,
									IPtrPlisgoFSData&	rData);

	virtual int			UnlockFile(	LONGLONG			nByteOffset,
									LONGLONG			nByteLength,
									IPtrPlisgoFSData&	rData);

	virtual int			SetEndOfFile(LONGLONG nEndPos, IPtrPlisgoFSData& rData);

	virtual int			Close(IPtrPlisgoFSData&	rData);

	bool				IsReadOnly() const							{ return m_bReadOnly; }
	void				SetReadOnly(bool bReadOnly);

	virtual bool		GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const;
	
private:
	bool							m_bReadOnly;
	bool							m_bWriteOpen;

	mutable boost::shared_mutex		m_Mutex;
};


class PlisgoFSRealFile : public PlisgoFSUserDataFile
{
public:

	PlisgoFSRealFile(const std::wstring& rsRealFile); 

	virtual bool			IsValid() const;

	virtual DWORD			GetAttributes() const;
	virtual LONGLONG		GetSize() const;

	virtual bool			GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const;


	virtual int				Open(	DWORD				nDesiredAccess,
									DWORD				nShareMode,
									DWORD				nCreationDisposition,
									DWORD				nFlagsAndAttributes,
									IPtrPlisgoFSData&	rData);

	virtual int				Read(	LPVOID				pBuffer,
									DWORD				nNumberOfBytesToRead,
									LPDWORD				pnNumberOfBytesRead,
									LONGLONG			nOffset,
									IPtrPlisgoFSData&	rData);

	virtual int				Write(	LPCVOID				pBuffer,
									DWORD				nNumberOfBytesToWrite,
									LPDWORD				pnNumberOfBytesWritten,
									LONGLONG			nOffset,
									IPtrPlisgoFSData&	rData);

	virtual int				LockFile(	LONGLONG			nByteOffset,
										LONGLONG			nByteLength,
										IPtrPlisgoFSData&	rData);

	virtual int				UnlockFile(	LONGLONG			nByteOffset,
										LONGLONG			nByteLength,
										IPtrPlisgoFSData&	rData);

	virtual int				GetHandleInfo(LPBY_HANDLE_FILE_INFORMATION pInfo, IPtrPlisgoFSData&	rData);

	virtual int				FlushBuffers(IPtrPlisgoFSData&	rData);

	virtual int				SetEndOfFile(LONGLONG nEndPos, IPtrPlisgoFSData&	rData);

	virtual int				GetDeleteError(IPtrPlisgoFSData&) const	{ return 0; }

	virtual int				Close(IPtrPlisgoFSData&	rData);

	virtual int				SetFileTimes(	const FILETIME*		pCreation,
											const FILETIME*		pLastAccess,
											const FILETIME*		pLastWrite,
											IPtrPlisgoFSData&	rData);

	virtual int				SetAttributes(DWORD	nFileAttributes, IPtrPlisgoFSData&	rData);

	const std::wstring&		GetRealPath() const		{ return m_sRealFile; }

private:

	std::wstring	m_sRealFile;
};



class PlisgoFSRealFolder : public PlisgoFSFolder
{
public:

	PlisgoFSRealFolder(const std::wstring& rsRealFolder); 

	virtual bool				IsValid() const;

	virtual DWORD				GetAttributes() const;

	virtual bool				GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const;

	virtual int					SetFileTimes(	const FILETIME*		pCreation,
												const FILETIME*		pLastAccess,
												const FILETIME*		pLastWrite,
												IPtrPlisgoFSData&	rData);

	virtual int					SetAttributes(DWORD	nFileAttributes, IPtrPlisgoFSData&	rData);

	virtual int 				GetChildren(ChildNames& rChildren) const;
	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sName) const;

	virtual int					AddChild(LPCWSTR sName, IPtrPlisgoFSFile file);

	virtual int					CreateChild(IPtrPlisgoFSFile& rChild, LPCWSTR sName, DWORD nDesiredAccess, DWORD nShareMode, DWORD nAttr, IPtrPlisgoFSData&	rData);

	virtual int					Repath(LPCWSTR sOldName, LPCWSTR sNewName, bool bReplaceExisting, PlisgoFSFolder* pNewParent);

	const std::wstring&			GetRealPath() const		{ return m_sRealPath; }

	virtual int					GetDeleteError(IPtrPlisgoFSData& rData) const;

	virtual int					GetRemoveChildError(LPCWSTR sName) const;

	virtual int					RemoveChild(LPCWSTR sName);
private:

	IPtrPlisgoFSFile			GetChild(	WIN32_FIND_DATAW& rFind) const;

	std::wstring				m_sRealPath;
};


//This class exists so a descendant class of PlisgoFSRealFolder can take ownership and have code called.
class PlisgoFSRealFolderTarget : public PlisgoFSRealFolder
{
public:

	PlisgoFSRealFolderTarget(const std::wstring& rsRealFolder) : PlisgoFSRealFolder(rsRealFolder)
	{}

	virtual int RepathTo(const std::wstring& rsFullSrc, LPCWSTR sNewName, bool bReplaceExisting) = 0;
};



class PlisgoFSEncapsulatedFile : public PlisgoFSFile
{
public:

	PlisgoFSEncapsulatedFile(IPtrPlisgoFSFile& rFile)
	{
		assert(rFile.get() != NULL);

		m_file = rFile;
	}

	void						Reset(IPtrPlisgoFSFile& rFile)			{ m_file = rFile; }

	virtual bool				IsValid() const							{ return m_file->IsValid(); }

	virtual DWORD				GetAttributes() const					{ return m_file->GetAttributes(); }

	virtual bool				GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const	{ return m_file->GetFileTimes(rCreation, rLastAccess, rLastWrite); }

	virtual LONGLONG			GetSize() const							{ return m_file->GetSize(); }

	virtual int					Open(	DWORD				nDesiredAccess,
										DWORD				nShareMode,
										DWORD				nCreationDisposition,
										DWORD				nFlagsAndAttributes,
										IPtrPlisgoFSData&	rData)		{ return m_file->Open(nDesiredAccess, nShareMode, nCreationDisposition, nFlagsAndAttributes, rData); }

	virtual int					Read(	LPVOID				pBuffer,
										DWORD				nNumberOfBytesToRead,
										LPDWORD				pnNumberOfBytesRead,
										LONGLONG			nOffset,
										IPtrPlisgoFSData&	rData)		{ return m_file->Read(pBuffer, nNumberOfBytesToRead, pnNumberOfBytesRead, nOffset, rData); }

	virtual int					Write(	LPCVOID				pBuffer,
										DWORD				nNumberOfBytesToWrite,
										LPDWORD				pnNumberOfBytesWritten,
										LONGLONG			nOffset,
										IPtrPlisgoFSData&	rData)		{ return m_file->Write(pBuffer, nNumberOfBytesToWrite, pnNumberOfBytesWritten, nOffset, rData); }

	virtual int					LockFile(	LONGLONG			nByteOffset,
											LONGLONG			nByteLength,
											IPtrPlisgoFSData&	rData)	{ return m_file->LockFile(nByteOffset, nByteLength, rData); }

	virtual int					UnlockFile(	LONGLONG			nByteOffset,
											LONGLONG			nByteLength,
											IPtrPlisgoFSData&	rData)	{ return m_file->UnlockFile(nByteOffset, nByteLength, rData); }

	virtual int					FlushBuffers(IPtrPlisgoFSData&	rData)	{ return m_file->FlushBuffers(rData); }

	virtual int					SetEndOfFile(LONGLONG nEndPos, IPtrPlisgoFSData& rData)	{ return m_file->SetEndOfFile(nEndPos, rData); }

	virtual int					Close(IPtrPlisgoFSData& rData)			{ return m_file->Close(rData); }

	virtual int					SetFileTimes(	const FILETIME*		pCreation,
												const FILETIME*		pLastAccess,
												const FILETIME*		pLastWrite,
												IPtrPlisgoFSData&	rData)	{ return m_file->SetFileTimes(pCreation, pLastAccess, pLastWrite, rData ); }

	virtual int					SetAttributes(	DWORD				nFileAttributes,
												IPtrPlisgoFSData&	rData)	{ return m_file->SetAttributes(nFileAttributes, rData ); }


	virtual PlisgoFSFolder*		GetAsFolder() const							{ return m_file->GetAsFolder(); }

	IPtrPlisgoFSFile			GetEncapsulated() const						{ return m_file; }

protected:
	IPtrPlisgoFSFile m_file;
};



inline IPtrPlisgoFSFile	GetPlisgoDesktopIniFile()
{
	std::string sData = "[.ShellClassInfo]\r\nCLSID={ADA19F85-EEB6-46F2-B8B2-2BD977934A79}\r\n";

	return IPtrPlisgoFSFile(new PlisgoFSStringHiddenReadOnly(sData));
}


class FileEvent
{
public:
	virtual ~FileEvent() {}

	//This dual method system is so if your don't need the path, you don't have it. This saves changes
	//existing code whilest still allowing new code that want the path to have it.
	virtual bool Do(IPtrPlisgoFSFile& rFile, const std::wstring& /*rsPath*/)	{ return Do(rFile); }
	virtual bool Do(IPtrPlisgoFSFile& /*rFile*/)								{ return true; }

};

typedef boost::shared_ptr<FileEvent>	IPtrFileEvent;