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

#include "PlisgoFSLib.h"
#include "PlisgoFSFiles.h"




int				PlisgoFSFile::Open(DWORD	nDesiredAccess,
									DWORD	,
									DWORD	nCreationDisposition,
									DWORD	,
									ULONGLONG*)
{
	if (nCreationDisposition == CREATE_NEW)
		return -ERROR_ALREADY_EXISTS;

	if (nDesiredAccess&ACCESS_SYSTEM_SECURITY)
		return -ERROR_PRIVILEGE_NOT_HELD;
/*
	if (nDesiredAccess&SYNCHRONIZE)
		return -ERROR_PRIVILEGE_NOT_HELD;

	if (nDesiredAccess&FILE_WRITE_ATTRIBUTES || nDesiredAccess&FILE_WRITE_EA)
		return -ERROR_PRIVILEGE_NOT_HELD;
*/
	return 0;
}


int				PlisgoFSFile::Read(	LPVOID		/*pBuffer*/,
										DWORD		/*nNumberOfBytesToRead*/,
										LPDWORD		/*pnNumberOfBytesRead*/,
										LONGLONG	/*nOffset*/	,
										ULONGLONG*	/*pInstanceData*/)							{ return -ERROR_ACCESS_DENIED; }

int				PlisgoFSFile::Write(	LPCVOID		/*pBuffer*/,
										DWORD		/*nNumberOfBytesToWrite*/,
										LPDWORD		/*pnNumberOfBytesWritten*/,
										LONGLONG	/*nOffset*/,
										ULONGLONG*	/*pInstanceData*/)							{ return -ERROR_ACCESS_DENIED; }

int				PlisgoFSFile::LockFile(	LONGLONG	,//nByteOffset,
											LONGLONG	,//nByteLength,
											ULONGLONG*	/*pInstanceData*/)						{ return -ERROR_INVALID_FUNCTION; };

int				PlisgoFSFile::UnlockFile(	LONGLONG	,//nByteOffset,
											LONGLONG	,//nByteLength,
											ULONGLONG*	/*pInstanceData*/)						{ return -ERROR_INVALID_FUNCTION; };

int				PlisgoFSFile::FlushBuffers(ULONGLONG* /*pInstanceData*/)						{ return 0; }

int				PlisgoFSFile::SetEndOfFile(LONGLONG /*nEndPos*/, ULONGLONG* /*pInstanceData*/){ return -ERROR_ACCESS_DENIED; }

int				PlisgoFSFile::Close(ULONGLONG* /*pInstanceData*/)								{ return 0; }

int				PlisgoFSFile::SetFileTimes(	const FILETIME* ,//pCreation,
												const FILETIME* ,//pLastAccess,
												const FILETIME* ,//pLastWrite
												ULONGLONG*		/*pInstanceData*/)					{ return -ERROR_INVALID_FUNCTION; }

int				PlisgoFSFile::SetFileAttributesW(	DWORD		,//nFileAttributes,
													ULONGLONG*	/*pInstanceData*/)					{ return -ERROR_INVALID_FUNCTION; }







template<typename T>
inline void GetFileInfo(const PlisgoFSFile* pThis, T* pFileInfo)
{
	assert(pFileInfo != NULL);

	ZeroMemory(pFileInfo, sizeof(T));

	pFileInfo->dwFileAttributes = pThis->GetAttributes();

	pThis->GetFileTimes(pFileInfo->ftCreationTime, pFileInfo->ftLastAccessTime, pFileInfo->ftLastWriteTime);

	LARGE_INTEGER nSize;

	nSize.QuadPart = pThis->GetSize();

	pFileInfo->nFileSizeHigh	= nSize.HighPart;
	pFileInfo->nFileSizeLow		= nSize.LowPart;
}


int	PlisgoFSFile::GetHandleInfo(LPBY_HANDLE_FILE_INFORMATION pInfo, ULONGLONG* )
{
	GetFileInfo(this, pInfo);

	return 0;
}


int	PlisgoFSFile::GetFindFileData(WIN32_FIND_DATAW* pFindData)
{
	assert(pFindData != NULL);

	ZeroMemory(pFindData, sizeof(WIN32_FIND_DATAW));

	GetFileInfo(this, pFindData);

	wcscpy_s(pFindData->cFileName, MAX_PATH-1, GetName());

	return 0;
}

/*
****************************************************************************
*/


IPtrPlisgoFSFile		PlisgoFSFolder::GetDescendant(LPCWSTR sPath) const
{
	assert(sPath != NULL);

	IPtrPlisgoFSFile file;

	{
		LPCWSTR sSlash = wcschr(sPath, L'\\');

		if (sSlash != NULL)
		{
			WCHAR sName[MAX_PATH];

			memcpy_s(sName, sizeof(WCHAR)*MAX_PATH, sPath, sizeof(WCHAR)*(sSlash-sPath));

			sName[sSlash-sPath] = L'\0';

			file = GetChild(sName);
		}
		else file = GetChild(sPath);

		if (file.get() == NULL)
			return IPtrPlisgoFSFile(); //No a valid path

		if (sSlash == NULL)
			return file; //Work here is done

		if (!(file->GetAttributes() & FILE_ATTRIBUTE_DIRECTORY))
			return IPtrPlisgoFSFile(); //No a valid path
		
		++sSlash;

		if (sSlash[0] == L'\0')
			return file; //Work here is done

		sPath = sSlash;
	}

	PlisgoFSFolder* pFolder = file->GetAsFolder();

	if (pFolder == NULL)
		return IPtrPlisgoFSFile(); //Not a folder

	return pFolder->GetDescendant(sPath);
}


class ChildCount : public PlisgoFSFolder::EachChild
{
public:
	ChildCount()	{ m_nChildNum = 0; }

	virtual bool Do(IPtrPlisgoFSFile ) { ++m_nChildNum; return true; }

	UINT	m_nChildNum;

};


UINT					PlisgoFSFolder::GetChildNum() const
{
	ChildCount counter;

	ForEachChild(counter);

	return counter.m_nChildNum;
}


/*
****************************************************************************
*/

void				PlisgoFSFileList::AddFile(IPtrPlisgoFSFile file)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	std::wstring sKey = file->GetName();

	std::transform(sKey.begin(),sKey.end(),sKey.begin(),tolower);

	m_files[sKey] = file;
}


bool				PlisgoFSFileList::ForEachFile(PlisgoFSFolder::EachChild& rEachFile) const
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	for(std::map<std::wstring, IPtrPlisgoFSFile >::const_iterator it = m_files.begin();
		it != m_files.end(); ++it)
	{
		if (!rEachFile.Do(it->second))
			return false;
	}

	return true;
}


IPtrPlisgoFSFile	PlisgoFSFileList::GetFile(LPCWSTR sName) const
{
	std::wstring sKey = sName;

	std::transform(sKey.begin(),sKey.end(),sKey.begin(),tolower);

	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	std::map<std::wstring, IPtrPlisgoFSFile >::const_iterator it = m_files.find(sKey);
	
	if (it != m_files.end())
		return it->second;

	return IPtrPlisgoFSFile();
}


IPtrPlisgoFSFile	PlisgoFSFileList::GetFile(UINT nIndex) const
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	if (nIndex >= m_files.size())
		return IPtrPlisgoFSFile();

	std::map<std::wstring, IPtrPlisgoFSFile >::const_iterator it = m_files.begin();

	while(nIndex--)
		++it;

	return it->second;
}


UINT				PlisgoFSFileList::GetLength() const
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	return (UINT)m_files.size();
}


IPtrPlisgoFSFile	PlisgoFSFileList::ParseName(LPCWSTR& rsPath) const
{
	assert(rsPath != NULL);

	if (rsPath[0] == L'\\')
		++rsPath;

	std::wstring sName;

	sName.reserve(MAX_PATH);

	for(LPCWSTR sSrc = rsPath; *sSrc != L'\\' && *sSrc != L'\0'; ++sSrc)
		sName.push_back(tolower(*sSrc));

	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);
		
	std::map<std::wstring, IPtrPlisgoFSFile >::const_iterator it = m_files.find(sName);
	
	if (it == m_files.end())
		return IPtrPlisgoFSFile();

	rsPath += sName.length();

	return it->second;
}
/*
****************************************************************************
*/
PlisgoFSDataFile::PlisgoFSDataFile(const std::wstring& rsPath, BYTE* pData, size_t nDataSize, bool bReadOnly, bool bCopy) : PlisgoFSStdPathFile(rsPath)
{
	m_pData = NULL;
	m_nDataSize = m_nDataUsedSize = 0;
	m_bOwnMemory = true;
	m_bWriteOpen = false;

	GetSystemTimeAsFileTime(&m_CreatedTime);
	m_LastWrite = m_CreatedTime;

	m_bReadOnly = bReadOnly;

	if (!bCopy)
	{
		m_pData = pData;
		m_nDataSize = m_nDataUsedSize = (ULONG)nDataSize;
		m_bOwnMemory = false;
	}
	else CreateOwnCopy(pData, nDataSize);
}


PlisgoFSDataFile::~PlisgoFSDataFile()
{
	if (m_bOwnMemory)
	{
		delete[] m_pData;
		m_pData = NULL;
		m_nDataSize = m_nDataUsedSize = 0;
	}
}


bool	PlisgoFSDataFile::GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const
{
	rCreation = m_CreatedTime;
	rLastWrite = m_LastWrite;

	GetSystemTimeAsFileTime(&rLastAccess);

	return true;
}


void	PlisgoFSDataFile::CreateOwnCopy(BYTE* pData, size_t nDataSize)
{
	m_bOwnMemory = true;

	m_nDataSize = 1024;

	while(m_nDataSize < nDataSize)
		m_nDataSize*=2;

	m_pData = new BYTE[m_nDataSize];

	if (m_pData != NULL && nDataSize != 0)
	{
		memcpy_s(m_pData, m_nDataSize, pData, nDataSize);

		m_nDataUsedSize = (ULONG)nDataSize;
	}
	else m_nDataUsedSize = 0;
}


void	PlisgoFSDataFile::SetReadOnly(bool bReadOnly)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	m_bReadOnly = bReadOnly;
}


int		PlisgoFSDataFile::Open(	DWORD		nDesiredAccess,
								DWORD		nShareMode,
								DWORD		nCreationDisposition,
								DWORD		nFlagsAndAttributes,
								ULONGLONG*	pInstanceData)
{
	assert(pInstanceData != NULL);

	if (nFlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return -ERROR_ACCESS_DENIED;

	int nError = PlisgoFSFile::Open(	nDesiredAccess,
										nShareMode,
										nCreationDisposition,
										nFlagsAndAttributes,
										pInstanceData);
	if (nError != 0)
		return nError;

	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	bool bWrite = (nDesiredAccess & GENERIC_WRITE || nDesiredAccess & GENERIC_ALL || nDesiredAccess & FILE_WRITE_DATA);

	if (bWrite && m_bReadOnly)
		return -ERROR_ACCESS_DENIED;


	if (bWrite)
	{
		boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

		if (m_bWriteOpen)
			return -ERROR_ACCESS_DENIED;

		m_bWriteOpen = true;

		if (nCreationDisposition == TRUNCATE_EXISTING || nCreationDisposition == CREATE_ALWAYS)
		{
			if (!m_bOwnMemory)
			{
				m_nDataSize = 1024;

				m_pData = new BYTE[m_nDataSize];

				assert(m_pData != NULL);

				m_nDataUsedSize = 0;

				m_bOwnMemory = true;
			}
			else m_nDataUsedSize = 0;
		}
		else if (!m_bOwnMemory)
			CreateOwnCopy(m_pData, m_nDataUsedSize);

		*pInstanceData = GENERIC_WRITE;
	}
	else *pInstanceData = GENERIC_READ;

	return 0;
}


int		PlisgoFSDataFile::SetEndOfFile(LONGLONG nEndPos, ULONGLONG* pInstanceData)
{
	assert(pInstanceData != NULL);

	assert(*pInstanceData == GENERIC_WRITE);

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	GrowToSize((size_t)nEndPos);

	GetSystemTimeAsFileTime(&m_LastWrite);

	return 0;
}


int		PlisgoFSDataFile::Close(ULONGLONG* pInstanceData)
{
	assert(pInstanceData != NULL);

	if (*pInstanceData == GENERIC_WRITE)
	{
		boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

		GetSystemTimeAsFileTime(&m_LastWrite);

		m_bWriteOpen = false;
	}

	*pInstanceData = 0;

	return 0;
}



int		PlisgoFSDataFile::Read(	LPVOID		pBuffer,
							DWORD		nNumberOfBytesToRead,
							LPDWORD		pnNumberOfBytesRead,
							LONGLONG	nOffset,
							ULONGLONG*	)
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	if (m_pData != NULL && nOffset < m_nDataUsedSize)
	{
		const DWORD	nLength	= (DWORD)(m_nDataUsedSize-nOffset);
		const DWORD	nCopySize = min(nLength,nNumberOfBytesToRead);

		memcpy_s(pBuffer, nNumberOfBytesToRead, &m_pData[nOffset], nCopySize);

		*pnNumberOfBytesRead = nCopySize;

		return 0;
	}
	else *pnNumberOfBytesRead = 0;

	return -ERROR_HANDLE_EOF;
}


void	PlisgoFSDataFile::GrowToSize(size_t nDataSize)
{
	if (m_nDataSize < nDataSize)
	{
		if (m_nDataSize != 0)
		{
			while(nDataSize > m_nDataSize)
				m_nDataSize*= 2;
		}
		else m_nDataSize = 1024;

		BYTE* pNewData = new BYTE[m_nDataSize];

		if (m_pData != NULL)
		{
			assert(pNewData != NULL);

			memcpy_s(pNewData, m_nDataSize, m_pData, m_nDataUsedSize);

			if (m_bOwnMemory)
				delete[] m_pData;
		}

		m_pData = pNewData;
		m_bOwnMemory = true;
	}
}


int		PlisgoFSDataFile::Write(	LPCVOID		pBuffer,
							DWORD		nNumberOfBytesToWrite,
							LPDWORD		pnNumberOfBytesWritten,
							LONGLONG	nOffset,
							ULONGLONG*	pInstanceData)
{
	assert(pInstanceData != NULL);

	if (!(*pInstanceData & GENERIC_WRITE))
		return -ERROR_ACCESS_DENIED;

	assert(pBuffer != NULL);

	if (m_bReadOnly)
		return -ERROR_ALREADY_EXISTS;

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	GrowToSize((size_t)(nOffset+nNumberOfBytesToWrite));

	memcpy_s((LPVOID)&m_pData[nOffset], (rsize_t)(m_nDataSize-nOffset), pBuffer, (rsize_t)nNumberOfBytesToWrite);

	*pnNumberOfBytesWritten = nNumberOfBytesToWrite;

	m_nDataUsedSize = (ULONG)nOffset + nNumberOfBytesToWrite;

	GetSystemTimeAsFileTime(&m_LastWrite);

	return 0;
}



/*
****************************************************************************
*/

PlisgoFSStringFile::PlisgoFSStringFile(	const std::wstring& rsPath,
							const std::wstring& rsData, 
							bool bReadOnly) : PlisgoFSStdPathFile(rsPath)
{
	SetString(rsData);

	m_bReadOnly = bReadOnly;
	m_bWriteOpen = false;
}



PlisgoFSStringFile::PlisgoFSStringFile(	const std::wstring& rsPath,
							const std::string& rsData, 
							bool bReadOnly) : PlisgoFSStdPathFile(rsPath)
{

	SetString(rsData);

	m_bReadOnly = bReadOnly;
	m_bWriteOpen = false;
}


void			PlisgoFSStringFile::SetString(const std::string& rsData)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	m_sData = rsData;
}


void			PlisgoFSStringFile::AddString(const std::string& rsData)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	m_sData+= rsData;
}


void			PlisgoFSStringFile::GetString(std::string& rsResult)
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	rsResult = m_sData;
}


void			PlisgoFSStringFile::SetString(const std::wstring& rsData)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	std::string sData2;

	FromWide(sData2, rsData);

	m_sData = sData2;
}


void			PlisgoFSStringFile::AddString(const std::wstring& rsData)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	std::string sData2;

	FromWide(sData2, rsData);

	m_sData+= sData2;
}


void			PlisgoFSStringFile::GetWideString(std::wstring& rResult)
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	ToWide(rResult, m_sData);
}


int				PlisgoFSStringFile::Open(	DWORD		nDesiredAccess,
									DWORD		nShareMode,
									DWORD		nCreationDisposition,
									DWORD		nFlagsAndAttributes,
									ULONGLONG*	pInstanceData)
{
	assert(pInstanceData != NULL);

	if (nFlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return -ERROR_ACCESS_DENIED;

	int nError = PlisgoFSFile::Open(	nDesiredAccess,
										nShareMode,
										nCreationDisposition,
										nFlagsAndAttributes,
										pInstanceData);
	if (nError != 0)
		return nError;

	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	bool bWrite = (nDesiredAccess & GENERIC_WRITE || nDesiredAccess & GENERIC_ALL || nDesiredAccess & FILE_WRITE_DATA);

	if (bWrite && m_bReadOnly)
		return -ERROR_ACCESS_DENIED;


	if (bWrite)
	{
		boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

		if (m_bWriteOpen)
			return -ERROR_ACCESS_DENIED;

		m_bWriteOpen = true;

		if (nCreationDisposition == TRUNCATE_EXISTING || nCreationDisposition == CREATE_ALWAYS)
			m_sData.resize(0);

		*pInstanceData = GENERIC_WRITE;
	}
	else *pInstanceData = GENERIC_READ;

	return 0;
}


int				PlisgoFSStringFile::SetEndOfFile(LONGLONG nEndPos, ULONGLONG* pInstanceData)
{
	assert(pInstanceData != NULL);

	assert(*pInstanceData == GENERIC_WRITE);

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	m_sData.resize((unsigned int)nEndPos);

	return 0;
}


int				PlisgoFSStringFile::Close(ULONGLONG* pInstanceData)
{
	assert(pInstanceData != NULL);

	if (*pInstanceData == GENERIC_WRITE)
	{
		boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

		m_bWriteOpen = false;
	}

	*pInstanceData = 0;

	return 0;
}


int				PlisgoFSStringFile::Read(LPVOID		pBuffer,
									DWORD		nNumberOfBytesToRead,
									LPDWORD		pnNumberOfBytesRead,
									LONGLONG	nOffset,
									ULONGLONG*	)
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	if (nOffset < m_sData.length())
	{
		const DWORD	nLength	= (DWORD)(m_sData.length()-nOffset);
		const DWORD	nCopySize = min(nLength,nNumberOfBytesToRead);

		memcpy_s(pBuffer, nNumberOfBytesToRead, &m_sData.c_str()[nOffset], nCopySize);

		*pnNumberOfBytesRead = nCopySize;

		return 0;
	}
	else *pnNumberOfBytesRead = 0;

	return -ERROR_HANDLE_EOF;
}


int				PlisgoFSStringFile::Write(LPCVOID		pBuffer,
									DWORD		nNumberOfBytesToWrite,
									LPDWORD		pnNumberOfBytesWritten,
									LONGLONG	nOffset,
									ULONGLONG*	pInstanceData)
{
	assert(pInstanceData != NULL);

	if (!(*pInstanceData & GENERIC_WRITE))
		return -ERROR_ACCESS_DENIED;

	assert(pBuffer != NULL);

	if (m_bReadOnly)
		return -ERROR_ALREADY_EXISTS;

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	LONGLONG nEndPos = nOffset+nNumberOfBytesToWrite;

	if (m_sData.capacity() < nEndPos)
		m_sData.reserve((unsigned int)(nEndPos)); //more then required

	m_sData.resize((unsigned int)nEndPos);

	memcpy_s((LPVOID)&m_sData.c_str()[nOffset], (rsize_t)(m_sData.capacity()-nOffset), pBuffer, (rsize_t)nNumberOfBytesToWrite);

	*pnNumberOfBytesWritten = nNumberOfBytesToWrite;

	return 0;
}



int				PlisgoFSStringFile::LockFile(LONGLONG	,//nByteOffset,
										LONGLONG	,//nByteLength,
										ULONGLONG*	/*pInstanceData*/)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (m_bReadOnly)
		return 0;

	return -ERROR_INVALID_FUNCTION;
}


int				PlisgoFSStringFile::UnlockFile(	LONGLONG	,//nByteOffset,
											LONGLONG	,//nByteLength,
											ULONGLONG*	/*pInstanceData*/)
{

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (m_bReadOnly)
		return 0;

	return -ERROR_INVALID_FUNCTION;
}



void			PlisgoFSStringFile::SetReadOnly(bool bReadOnly)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	m_bReadOnly = bReadOnly;
}


LONGLONG		PlisgoFSStringFile::GetSize() const
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	return m_sData.length();
}



/*
****************************************************************************
*/


PlisgoFSRedirectionFile::PlisgoFSRedirectionFile(const std::wstring& rsPath,
									 const std::wstring& rsRealFile) : PlisgoFSStdPathFile<PlisgoFSFile>::PlisgoFSStdPathFile(rsPath)
{
	m_sRealFile = rsRealFile;
}


DWORD			PlisgoFSRedirectionFile::GetAttributes() const
{
	return GetFileAttributesW(m_sRealFile.c_str());
}


LONGLONG		PlisgoFSRedirectionFile::GetSize() const
{
	WIN32_FILE_ATTRIBUTE_DATA  data = {0};

	GetFileAttributesExW(m_sRealFile.c_str(), GetFileExInfoStandard, &data);

	LARGE_INTEGER temp;

	temp.LowPart = data.nFileSizeLow;
	temp.HighPart = data.nFileSizeHigh;

	return temp.QuadPart;
}


bool			PlisgoFSRedirectionFile::GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const
{
	WIN32_FILE_ATTRIBUTE_DATA  data = {0};

	if (!GetFileAttributesExW(m_sRealFile.c_str(), GetFileExInfoStandard, &data))
		return false;

	rCreation	= data.ftCreationTime;
	rLastAccess	= data.ftLastAccessTime;
	rLastWrite	= data.ftLastWriteTime;

	return true;
}


int				PlisgoFSRedirectionFile::GetHandleInfo(LPBY_HANDLE_FILE_INFORMATION pInfo, ULONGLONG* pInstanceData)
{
	assert(pInstanceData != NULL);

	HANDLE hFile = *(HANDLE*)pInstanceData;

	if(!GetFileInformationByHandle(hFile, pInfo))
		return -(int)GetLastError();

	return 0;
}

int				PlisgoFSRedirectionFile::Open(	DWORD		nDesiredAccess,
												DWORD		nShareMode,
												DWORD		nCreationDisposition,
												DWORD		nFlagsAndAttributes,
												ULONGLONG*	pInstanceData)
{
	assert(pInstanceData != NULL);

	const DWORD nAttr = GetFileAttributesW(m_sRealFile.c_str());

	if (nAttr != INVALID_FILE_ATTRIBUTES  && nAttr & FILE_ATTRIBUTE_DIRECTORY)
		nFlagsAndAttributes |= FILE_FLAG_BACKUP_SEMANTICS;

	HANDLE hFile = CreateFileW(	m_sRealFile.c_str(),
								nDesiredAccess,
								nShareMode,
								NULL,
								nCreationDisposition,
								nFlagsAndAttributes, NULL);

	if (hFile != NULL && hFile != INVALID_HANDLE_VALUE)
	{
		*(HANDLE*)pInstanceData = hFile;

		return 0;
	}
	else return -(int)GetLastError();
}


static bool		SetFilePosition(HANDLE hFile, LONGLONG	nOffset)
{
	LARGE_INTEGER temp;

	temp.QuadPart = nOffset;

	return (SetFilePointerEx(hFile, temp, NULL, FILE_BEGIN) == TRUE);
}


int				PlisgoFSRedirectionFile::Read(LPVOID		pBuffer,
										DWORD		nNumberOfBytesToRead,
										LPDWORD		pnNumberOfBytesRead,
										LONGLONG	nOffset,
										ULONGLONG*	pInstanceData)
{
	assert(pInstanceData != NULL);

	HANDLE hFile = *(HANDLE*)pInstanceData;

	if (!SetFilePosition(hFile, nOffset))
		return -(int)GetLastError();

	if (ReadFile(hFile, pBuffer, nNumberOfBytesToRead, pnNumberOfBytesRead, NULL))
		return 0;
	else
		return -(int)GetLastError();
}


int				PlisgoFSRedirectionFile::Write(	LPCVOID		pBuffer,
											DWORD		nNumberOfBytesToWrite,
											LPDWORD		pnNumberOfBytesWritten,
											LONGLONG	nOffset,
											ULONGLONG*	pInstanceData)
{
	assert(pInstanceData != NULL);

	HANDLE hFile = *(HANDLE*)pInstanceData;

	if (!SetFilePosition(hFile, nOffset))
		return -(int)GetLastError();

	if (WriteFile(hFile, pBuffer, nNumberOfBytesToWrite, pnNumberOfBytesWritten, NULL))
		return 0;
	else
		return -(int)GetLastError();
}


int				PlisgoFSRedirectionFile::FlushBuffers(ULONGLONG* pInstanceData)
{
	assert(pInstanceData != NULL);

	HANDLE hFile = *(HANDLE*)pInstanceData;

	if (FlushFileBuffers(hFile))
		return 0;
	else
		return -(int)GetLastError();
}


int				PlisgoFSRedirectionFile::SetEndOfFile(LONGLONG nEndPos, ULONGLONG* pInstanceData)
{
	assert(pInstanceData != NULL);

	HANDLE hFile = *(HANDLE*)pInstanceData;

	if (!SetFilePosition(hFile, nEndPos))
		return -(int)GetLastError();

	if (::SetEndOfFile(hFile))
		return 0;
	else
		return -(int)GetLastError();
}


int				PlisgoFSRedirectionFile::Close(ULONGLONG* pInstanceData)
{
	assert(pInstanceData != NULL);

	HANDLE hFile = *(HANDLE*)pInstanceData;

	if (CloseHandle(hFile))
		return 0;
	else
		return -(int)GetLastError();
}




int				PlisgoFSRedirectionFile::LockFile(LONGLONG	nByteOffset,
											LONGLONG	nByteLength,
											ULONGLONG*	pInstanceData)
{
	HANDLE hFile = *(HANDLE*)pInstanceData;

	LARGE_INTEGER fileOffset, fileLength;

	fileOffset.QuadPart = nByteOffset;
	fileLength.QuadPart = nByteLength;

	if (!::LockFile(hFile, fileOffset.LowPart, fileOffset.HighPart, fileLength.LowPart, fileLength.HighPart))
		return -(int)GetLastError();

	return 0;
}


int				PlisgoFSRedirectionFile::UnlockFile(	LONGLONG	nByteOffset,
												LONGLONG	nByteLength,
												ULONGLONG*	pInstanceData)
{
	HANDLE hFile = *(HANDLE*)pInstanceData;

	LARGE_INTEGER fileOffset, fileLength;

	fileOffset.QuadPart = nByteOffset;
	fileLength.QuadPart = nByteLength;

	if (!::UnlockFile(hFile, fileOffset.LowPart, fileOffset.HighPart, fileLength.LowPart, fileLength.HighPart))
		return -(int)GetLastError();

	return 0;
}


int				PlisgoFSRedirectionFile::SetFileTimes(const FILETIME* pCreation,
												const FILETIME* pLastAccess,
												const FILETIME* pLastWrite,
												ULONGLONG*		pInstanceData)
{
	HANDLE hFile = *(HANDLE*)pInstanceData;

	if(!::SetFileTime(hFile, pCreation, pLastAccess, pLastWrite))
		return -(int)GetLastError();

	return 0;
}


int				PlisgoFSRedirectionFile::SetFileAttributesW(DWORD	nFileAttributes, ULONGLONG* )
{
	if (!::SetFileAttributesW(m_sRealFile.c_str(), nFileAttributes))
		return -(int)GetLastError();

	return 0;
}


/*
****************************************************************************
*/


class ManagedEncapsulatedFile : public EncapsulatedFile, public boost::enable_shared_from_this<ManagedEncapsulatedFile>
{
public:
	ManagedEncapsulatedFile(const std::wstring& rsFullPathLowerCase,
							PlisgoFSCachedFileTree* pCacheHolder,
							IPtrPlisgoFSFile file) : EncapsulatedFile(file)
	{
		m_pCacheHolder	= pCacheHolder;
		m_sFullPath		= rsFullPathLowerCase;
	}

	int					Open(	DWORD		nDesiredAccess,
								DWORD		nShareMode,
								DWORD		nCreationDisposition,
								DWORD		nFlagsAndAttributes,
								ULONGLONG*	pInstanceData)
	{
		int nError = EncapsulatedFile::Open(nDesiredAccess, nShareMode, nCreationDisposition, nFlagsAndAttributes, pInstanceData);

		if (nError == 0)
			m_pCacheHolder->LogOpening(m_sFullPath, shared_from_this());

		return nError;
	}

	int					Close(ULONGLONG* pInstanceData)
	{
		m_pCacheHolder->LogClosing(m_sFullPath, shared_from_this());

		return EncapsulatedFile::Close(pInstanceData);
	}

private:
	PlisgoFSCachedFileTree*	m_pCacheHolder;
	std::wstring			m_sFullPath;
};


/*
****************************************************************************
*/

IPtrPlisgoFSFile	PlisgoFSCachedFileTree::TracePath(LPCWSTR sPath, bool* pbInTrees) const
{
	std::wstring sOrgPathLowerCase = sPath;

	std::transform(sOrgPathLowerCase.begin(),sOrgPathLowerCase.end(),sOrgPathLowerCase.begin(),tolower);

	if (sOrgPathLowerCase.length() && sOrgPathLowerCase[sOrgPathLowerCase.length()-1] == L'\\')
		sOrgPathLowerCase.resize(sOrgPathLowerCase.length()-1);

	assert(sPath != NULL);

	if (pbInTrees != NULL)
		*pbInTrees = false;

	if (sPath == NULL)
		return IPtrPlisgoFSFile();

	IPtrPlisgoFSFile result = FindLoggedOpen(sOrgPathLowerCase);

	if (result.get() != NULL)
	{
		if (pbInTrees != NULL)
			*pbInTrees = true;

		return result;
	}
	
	IPtrPlisgoFSFile file = ParseName(sPath);

	if (file.get() == NULL)
		return IPtrPlisgoFSFile();

	if (sPath[0] == L'\0')
	{
		if (pbInTrees != NULL)
			*pbInTrees = true;

		return file;
	}

	if (file->GetAsFolder() == NULL)
		return IPtrPlisgoFSFile();

	if (sPath[0] == L'\0')
	{
		if (pbInTrees != NULL)
			*pbInTrees = true;

		return file;
	}
	else if (sPath[0] != L'\\')	
		return IPtrPlisgoFSFile();
	else
		++sPath;

	if (pbInTrees != NULL)
		*pbInTrees = true;

	if (sPath[0] == L'\0')
		return file;

	result = file->GetAsFolder()->GetDescendant(sPath);

	if (result.get() == NULL)
		return result;

	result.reset(new ManagedEncapsulatedFile(sOrgPathLowerCase, const_cast<PlisgoFSCachedFileTree*>(this), result));

	return result;
}


bool				PlisgoFSCachedFileTree::IncrementValidCacheEntry(const std::wstring& rsFullPathLowerCase, IPtrPlisgoFSFile file)
{
	OpenFilesMap::iterator it = m_OpenFiles.find(rsFullPathLowerCase);

	if (it == m_OpenFiles.end())
		return false;
	
	if (InterlockedIncrement(&it->second.nOpenNum) != 1)
		return true;
	
	//It was zero....

	return (it->second.file == file);
}


void				PlisgoFSCachedFileTree::LogOpening(const std::wstring& rsFullPathLowerCase, IPtrPlisgoFSFile file)
{
	if (file.get() == NULL)
		return;

	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	if (IncrementValidCacheEntry(rsFullPathLowerCase, file))
		return;

	boost::upgrade_to_unique_lock<boost::shared_mutex> rwlock(lock);

	if (IncrementValidCacheEntry(rsFullPathLowerCase, file))
		return;

	OpenFile& rOpenFile = m_OpenFiles[rsFullPathLowerCase];

	rOpenFile.file = file;
	rOpenFile.nOpenNum = 1;
}


IPtrPlisgoFSFile	PlisgoFSCachedFileTree::FindLoggedOpen(const std::wstring& rsFullPathLowerCase) const
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	OpenFilesMap::const_iterator it = m_OpenFiles.find(rsFullPathLowerCase);

	if (it != m_OpenFiles.end())
		return it->second.file;

	return IPtrPlisgoFSFile();
}


void				PlisgoFSCachedFileTree::LogClosing(const std::wstring& rsFullPathLowerCase, IPtrPlisgoFSFile file)
{
	if (file.get() == NULL)
		return;

	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	OpenFilesMap::iterator it = m_OpenFiles.find(rsFullPathLowerCase);

	if (it != m_OpenFiles.end())
		if (InterlockedDecrement(&it->second.nOpenNum) == 0)
		{
			boost::upgrade_to_unique_lock<boost::shared_mutex> rwlock(lock);

			it = m_OpenFiles.find(rsFullPathLowerCase);

			if (it != m_OpenFiles.end() && it->second.nOpenNum == 0)
				m_OpenFiles.erase(rsFullPathLowerCase);			
		}
}


