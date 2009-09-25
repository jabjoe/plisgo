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

	virtual LPCWSTR				GetName() const = 0;
	virtual const std::wstring&	GetPath() const = 0;

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

	virtual int					SetFileTimes(	const FILETIME* pCreation,
												const FILETIME* pLastAccess,
												const FILETIME* pLastWrite,
												ULONGLONG*		pInstanceData);

	virtual int					SetFileAttributesW(	DWORD		nFileAttributes,
													ULONGLONG*	pInstanceData);

	virtual PlisgoFSFolder*		GetAsFolder() const			{ return NULL; }

	virtual int					GetHandleInfo(LPBY_HANDLE_FILE_INFORMATION pInfo, ULONGLONG* pInstanceData);
	int							GetFindFileData(WIN32_FIND_DATAW* pFindData);

};


typedef boost::shared_ptr<PlisgoFSFile>	IPtrPlisgoFSFile;



class PlisgoFSFolder : public PlisgoFSFile
{
public:
	virtual DWORD				GetAttributes() const						{ return FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY; }
	virtual LONGLONG			GetSize() const								{ return 0; }

	class EachChild
	{
	public:
		virtual bool Do(IPtrPlisgoFSFile file) = 0;
	};

	virtual IPtrPlisgoFSFile	GetDescendant(LPCWSTR sPath) const;

	virtual bool				ForEachChild(EachChild& rEachChild) const = 0;
	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sName) const = 0;

	virtual UINT				GetChildNum() const;

	PlisgoFSFolder*				GetAsFolder() const								{ return const_cast<PlisgoFSFolder*>(this); }
};

typedef boost::shared_ptr<PlisgoFSFolder>	IPtrPlisgoFSFolder;



template<class BASE>
class PlisgoFSStdPathFile : public BASE
{
public:
	PlisgoFSStdPathFile(const std::wstring& rsPath)								{ m_sPath = rsPath; }

	virtual LPCWSTR				GetName() const								{ return GetNameFromPath(m_sPath); }
	virtual const std::wstring&	GetPath() const								{ return m_sPath; }

protected:
	std::wstring				m_sPath;
};


class PlisgoFSFileList
{
public:

	void				AddFile(IPtrPlisgoFSFile file);
	bool				ForEachFile(PlisgoFSFolder::EachChild& rEachFile) const;
	IPtrPlisgoFSFile	ParseName(LPCWSTR& rsPath) const;
	IPtrPlisgoFSFile	GetFile(LPCWSTR sName) const;
	IPtrPlisgoFSFile	GetFile(UINT nIndex) const;
	UINT				GetLength() const;

private:
	mutable boost::shared_mutex					m_Mutex;
	
	std::map<std::wstring, IPtrPlisgoFSFile >	m_files;	
};



class PlisgoFSStorageFolder : public PlisgoFSStdPathFile<PlisgoFSFolder>
{
public:

	PlisgoFSStorageFolder(const std::wstring& rsPath) : PlisgoFSStdPathFile<PlisgoFSFolder>::PlisgoFSStdPathFile(rsPath)
	{
	}

	virtual bool				ForEachChild(PlisgoFSFolder::EachChild& rEachChild) const
	{
		return m_childList.ForEachFile(rEachChild);
	}

	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sName) const
	{
		return m_childList.GetFile(sName);
	}

	virtual UINT				GetChildNum() const
	{
		return m_childList.GetLength();
	}

	void						AddChild(IPtrPlisgoFSFile file)
	{
		m_childList.AddFile(file);
	}

protected:
	PlisgoFSFileList		m_childList;
};





class PlisgoFSDataFile : public PlisgoFSStdPathFile<PlisgoFSFile>
{
public:
	PlisgoFSDataFile(	const std::wstring&	rsPath,
						BYTE*				pData = NULL,
						size_t				nDataSize = 0,
						bool				bReadOnly = true,
						bool				bCopy = false);
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



class PlisgoFSStringFile : public  PlisgoFSStdPathFile<PlisgoFSFile>
{
public:
	PlisgoFSStringFile(	const std::wstring& rsPath) :  PlisgoFSStdPathFile<PlisgoFSFile>(rsPath) { m_bReadOnly = m_bWriteOpen = false; }

	PlisgoFSStringFile(	const std::wstring& sPath,
						const std::wstring& sData, 
						bool				bReadOnly = false);

	PlisgoFSStringFile(	const std::wstring&	sPath,
						const std::string& sData, 
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




class PlisgoFSRedirectionFile : public PlisgoFSStdPathFile<PlisgoFSFile>
{
public:

	PlisgoFSRedirectionFile(const std::wstring& rsPath, const std::wstring& rsRealFile); 


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

	virtual int				Close(ULONGLONG* pInstanceData);

	virtual int				SetFileTimes(	const FILETIME* pCreation,
											const FILETIME* pLastAccess,
											const FILETIME*	pLastWrite,
											ULONGLONG*		pInstanceData);

	virtual int				SetFileAttributesW(DWORD	nFileAttributes, ULONGLONG* pInstanceData);

	const std::wstring&		GetRealPath() const		{ return m_sRealFile; }

private:

	std::wstring	m_sRealFile;
};



class EncapsulatedFile : public PlisgoFSStdPathFile<PlisgoFSFile>
{
public:

	EncapsulatedFile(IPtrPlisgoFSFile& rFile) : PlisgoFSStdPathFile<PlisgoFSFile>::PlisgoFSStdPathFile(rFile->GetPath())
	{
		m_file = rFile;
	}

	EncapsulatedFile(const std::wstring& rsPath, IPtrPlisgoFSFile& rFile) : PlisgoFSStdPathFile<PlisgoFSFile>::PlisgoFSStdPathFile(rsPath)
	{
		m_file = rFile;
	}

	void						Reset(IPtrPlisgoFSFile& rFile)			{ m_file = rFile; }

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

	virtual int					SetFileAttributesW(	DWORD		nFileAttributes,
													ULONGLONG*	pInstanceData)	{ return m_file->SetFileAttributesW(nFileAttributes, pInstanceData ); }


	PlisgoFSFolder*				GetAsFolder() const						{ return m_file->GetAsFolder(); }

protected:
	IPtrPlisgoFSFile m_file;
};



class PlisgoFSCachedFileTree : public PlisgoFSFileList
{
	friend class ManagedEncapsulatedFile;

public:

	IPtrPlisgoFSFile			TracePath(LPCWSTR sPath, bool* pbInTrees = NULL) const;

protected:

	void						LogOpening(const std::wstring& rsFullPathLowerCase, IPtrPlisgoFSFile file);
	void						LogClosing(const std::wstring& rsFullPathLowerCase, IPtrPlisgoFSFile file);
private:

	IPtrPlisgoFSFile			FindLoggedOpen(const std::wstring& rsFullPathLowerCase) const;

	bool						IncrementValidCacheEntry(const std::wstring& rsKey, IPtrPlisgoFSFile file);

	struct OpenFile
	{
		IPtrPlisgoFSFile	file;
		volatile LONG		nOpenNum;
	};

	typedef std::map<std::wstring, OpenFile>	OpenFilesMap;


	mutable boost::shared_mutex		m_Mutex;
	OpenFilesMap					m_OpenFiles;
};


typedef boost::shared_ptr<PlisgoFSCachedFileTree>	IPtrPlisgoFSCachedFileTree;




inline IPtrPlisgoFSFile	GetPlisgoDesktopIniFile(const std::wstring& rsFolder)
{
	std::string sData = "[.ShellClassInfo]\r\nCLSID={ADA19F85-EEB6-46F2-B8B2-2BD977934A79}\r\n";

	return IPtrPlisgoFSFile(new PlisgoFSStringFile(rsFolder + L"\\Desktop.ini", sData, true));
}

