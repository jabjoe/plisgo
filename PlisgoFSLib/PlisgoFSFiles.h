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

	virtual bool				IsValid() const				{ return true; }

	virtual DWORD				GetAttributes() const		{ return FILE_ATTRIBUTE_READONLY; }

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
	virtual DWORD				GetAttributes() const						{ return FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_READONLY; }
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

	virtual int					CreateChild(IPtrPlisgoFSFile& rChild, LPCWSTR sName, DWORD nAttr, ULONGLONG* pInstanceData)	{ return -ERROR_ACCESS_DENIED; }

	virtual int					Repath(LPCWSTR sOldName, LPCWSTR sNewName, bool bReplaceExisting, PlisgoFSFolder* pNewParent);

	virtual int					GetRemoveChildError(LPCWSTR sName) const		{ return -ERROR_ACCESS_DENIED; }
	virtual int					RemoveChild(LPCWSTR sName)						{ return -ERROR_ACCESS_DENIED; }

	PlisgoFSFolder*				GetAsFolder() const								{ return const_cast<PlisgoFSFolder*>(this); }
};

typedef boost::shared_ptr<PlisgoFSFolder>	IPtrPlisgoFSFolder;


struct iequal_to : std::binary_function<std::wstring, std::wstring, bool>
{
    bool operator()(std::wstring const& x,
        std::wstring const& y) const
    {
        return boost::algorithm::iequals(x, y, std::locale());
    }
};

struct ihash : std::unary_function<std::wstring, std::size_t>
{
    std::size_t operator()(std::wstring const& x) const
    {
        std::size_t seed = 0;
        std::locale locale;

        for(std::wstring::const_iterator it = x.begin();
            it != x.end(); ++it)
        {
            boost::hash_combine(seed, std::toupper(*it, locale));
        }

        return seed;
    }
};

typedef boost::unordered_map<std::wstring, IPtrPlisgoFSFile, ihash, iequal_to>	PlisgoFSFileMap;
typedef std::vector<IPtrPlisgoFSFile>											PlisgoFSFileArray;

class PlisgoFSFileList
{
public:

	void				AddFile(LPCWSTR sName, IPtrPlisgoFSFile file);
	void				RemoveFile(LPCWSTR sName);
	bool				ForEachFile(PlisgoFSFolder::EachChild& rEachFile) const;
	IPtrPlisgoFSFile	GetFile(LPCWSTR sName) const;
	IPtrPlisgoFSFile	GetFile(UINT nIndex) const;
	UINT				GetLength() const;
	void				Clear();

private:
	
	mutable boost::shared_mutex		m_Mutex;
	PlisgoFSFileMap					m_Files;
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

	virtual int					CreateChild(IPtrPlisgoFSFile& rChild, LPCWSTR sName, DWORD nAttr, ULONGLONG* pInstanceData);

	virtual int					GetRemoveChildError(LPCWSTR sName) const		{ return 0; }
	virtual int					RemoveChild(LPCWSTR sName)						{ m_childList.RemoveFile(sName); return 0; }

protected:
	PlisgoFSFileList		m_childList;
};


class PlisgoFSReadOnlyStorageFolder : public PlisgoFSFolder
{
public:

	PlisgoFSReadOnlyStorageFolder(const PlisgoFSFileMap& rFileMap);

	virtual bool				ForEachChild(PlisgoFSFolder::EachChild& rEachChild) const;

	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sName) const;

	virtual int					AddChild(LPCWSTR , IPtrPlisgoFSFile )							{ return -ERROR_ACCESS_DENIED; }
	virtual int					CreateChild(IPtrPlisgoFSFile& , LPCWSTR , DWORD, ULONGLONG*)	{ return -ERROR_ACCESS_DENIED; }
	virtual int					GetRemoveChildError(LPCWSTR ) const								{ return -ERROR_ACCESS_DENIED; }
	virtual int					RemoveChild(LPCWSTR )											{ return -ERROR_ACCESS_DENIED; }

private:

	PlisgoFSFileMap		m_Files;
};


class PlisgoFSDataFileReadOnly : public PlisgoFSFile
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

private:
	BYTE*			m_pData;
	size_t			m_nDataSize;
	bool			m_bOwnMemory;
	bool			m_bFreeNotDelete;
	FILETIME		m_Time;
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

	bool					IsValid() const									{ return !m_bVolatile; }

	bool					SetVolatile(bool bVolatile)						{ m_bVolatile = bVolatile; }

	virtual DWORD			GetAttributes() const							{ return ((m_bReadOnly)?FILE_ATTRIBUTE_READONLY:0); }
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
	bool						m_bVolatile;

	FILETIME					m_CreatedTime;
	FILETIME					m_LastWrite;

};


class PlisgoFSStringReadOnly : public  PlisgoFSFile
{
public:
	PlisgoFSStringReadOnly()			{ m_bVolatile = false; }
	PlisgoFSStringReadOnly(	const std::wstring& sData);
	PlisgoFSStringReadOnly(	const std::string& sData);

	bool				IsValid() const									{ return !m_bVolatile; }
	void				SetVolatile(bool bVolatile)						{ m_bVolatile = bVolatile; }

	virtual void		GetWideString(std::wstring& rResult)			{ ToWide(rResult, m_sData); }
	virtual void		GetString(std::string& rsResult)				{ rsResult = m_sData; }

	virtual DWORD		GetAttributes() const							{ return FILE_ATTRIBUTE_READONLY; }
	virtual LONGLONG	GetSize() const									{ return m_sData.size(); }

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

	virtual int			LockFile(	LONGLONG, LONGLONG ,ULONGLONG*	)	{ return 0; }
	virtual int			UnlockFile(	LONGLONG, LONGLONG ,ULONGLONG*	)	{ return 0; }

	virtual int			Close(ULONGLONG* pInstanceData);

protected:
	volatile bool					m_bVolatile;
	std::string						m_sData;
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

	mutable boost::shared_mutex		m_Mutex;
};




class PlisgoFSRealFile : public PlisgoFSFile
{
public:

	PlisgoFSRealFile(const std::wstring& rsRealFile); 

	virtual bool			IsValid() const;

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



class PlisgoFSRealFolder : public PlisgoFSFolder
{
public:

	PlisgoFSRealFolder(const std::wstring& rsRealFolder); 

	virtual bool				IsValid() const;

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
	virtual int					CreateChild(IPtrPlisgoFSFile& rChild, LPCWSTR sName, DWORD nAttr, ULONGLONG* pInstanceData);

	virtual int					Repath(LPCWSTR sOldName, LPCWSTR sNewName, bool bReplaceExisting, PlisgoFSFolder* pNewParent);

	const std::wstring&			GetRealPath() const		{ return m_sRealPath; }

	virtual int					GetDeleteError(ULONGLONG* pInstanceData) const;

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
		m_file = rFile;
	}

	void						Reset(IPtrPlisgoFSFile& rFile)			{ m_file = rFile; }

	virtual bool				IsValid() const							{ return m_file->IsValid(); }

	virtual DWORD				GetAttributes() const					{ return m_file->GetAttributes(); }

	virtual bool				GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const	{ return m_file->GetFileTimes(rCreation, rLastAccess, rLastWrite); }

	virtual LONGLONG			GetSize() const							{ return m_file->GetSize(); }

	virtual int					Open(	DWORD		nDesiredAccess,
										DWORD		nShareMode,
										DWORD		nCreationDisposition,
										DWORD		nFlagsAndAttributes,
										ULONGLONG*	pInstanceData)		{ return m_file->Open(nDesiredAccess, nShareMode, nCreationDisposition, nFlagsAndAttributes, pInstanceData); }

	virtual int					Read(	LPVOID		pBuffer,
										DWORD		nNumberOfBytesToRead,
										LPDWORD		pnNumberOfBytesRead,
										LONGLONG	nOffset,
										ULONGLONG*	pInstanceData)		{ return m_file->Read(pBuffer, nNumberOfBytesToRead, pnNumberOfBytesRead, nOffset, pInstanceData); }

	virtual int					Write(	LPCVOID		pBuffer,
										DWORD		nNumberOfBytesToWrite,
										LPDWORD		pnNumberOfBytesWritten,
										LONGLONG	nOffset,
										ULONGLONG*	pInstanceData)		{ return m_file->Write(pBuffer, nNumberOfBytesToWrite, pnNumberOfBytesWritten, nOffset, pInstanceData); }

	virtual int					LockFile(	LONGLONG	nByteOffset,
											LONGLONG	nByteLength,
											ULONGLONG*	pInstanceData)	{ return m_file->LockFile(nByteOffset, nByteLength, pInstanceData); }

	virtual int					UnlockFile(	LONGLONG	nByteOffset,
											LONGLONG	nByteLength,
											ULONGLONG*	pInstanceData)	{ return m_file->UnlockFile(nByteOffset, nByteLength, pInstanceData); }

	virtual int					FlushBuffers(ULONGLONG* pInstanceData)	{ return m_file->FlushBuffers(pInstanceData); }

	virtual int					SetEndOfFile(LONGLONG nEndPos, ULONGLONG* pInstanceData)	{ return m_file->SetEndOfFile(nEndPos, pInstanceData); }

	virtual int					Close(ULONGLONG* pInstanceData)			{ return m_file->Close(pInstanceData); }

	virtual int					SetFileTimes(	const FILETIME* pCreation,
												const FILETIME* pLastAccess,
												const FILETIME* pLastWrite,
												ULONGLONG*		pInstanceData)	{ return m_file->SetFileTimes(pCreation, pLastAccess, pLastWrite, pInstanceData ); }

	virtual int					SetAttributes(	DWORD		nFileAttributes,
												ULONGLONG*	pInstanceData)	{ return m_file->SetAttributes(nFileAttributes, pInstanceData ); }


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
	virtual bool Do(IPtrPlisgoFSFile& rFile, const std::wstring& rsPath)	{ return Do(rFile); }
	virtual bool Do(IPtrPlisgoFSFile& rFile)								{ return true; }

};

typedef boost::shared_ptr<FileEvent>	IPtrFileEvent;