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

int				PlisgoFSFile::SetAttributes(	DWORD		,//nFileAttributes,
												ULONGLONG*	/*pInstanceData*/)					{ return -ERROR_INVALID_FUNCTION; }



int	PlisgoFSFile::GetHandleInfo(LPBY_HANDLE_FILE_INFORMATION pInfo, ULONGLONG* )
{
	GetFileInfo(pInfo);

	return 0;
}

/*
****************************************************************************
*/


int				PlisgoFSFolder::Open(	DWORD	nDesiredAccess,
										DWORD	,
										DWORD	nCreationDisposition,
										DWORD	,
										ULONGLONG*)
{
	if (nCreationDisposition == CREATE_NEW || nCreationDisposition == TRUNCATE_EXISTING)
		return -ERROR_ALREADY_EXISTS;

	if (nDesiredAccess&(GENERIC_WRITE|FILE_WRITE_DATA|FILE_APPEND_DATA|FILE_WRITE_EA))
		return -ERROR_ACCESS_DENIED;

	return 0;
}


class FolderCopier : public PlisgoFSFolder::EachChild
{
public:

	FolderCopier(PlisgoFSFolder* pDstFolder)
	{
		m_pDstFolder	= pDstFolder;
		m_nError		= 0;
	}

	virtual bool Do(LPCWSTR sName, IPtrPlisgoFSFile file);

	int	GetError() const { return m_nError; }

private:

	PlisgoFSFolder* m_pDstFolder;
	int				m_nError;
};




static int				CopyPlisgoFile(IPtrPlisgoFSFile& rSrcFile, IPtrPlisgoFSFile& rDstFile)
{
	if (rSrcFile->GetAttributes()&FILE_ATTRIBUTE_DIRECTORY)
	{
		if ((rDstFile->GetAttributes()&FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
			return -ERROR_BAD_PATHNAME;

		PlisgoFSFolder* pSrcChild = rSrcFile->GetAsFolder();
		PlisgoFSFolder* pDstChild = rDstFile->GetAsFolder();

		if (pSrcChild == NULL || pDstChild == NULL)
			return -ERROR_BAD_PATHNAME;

		FolderCopier copier(pDstChild);

		pSrcChild->ForEachChild(copier);

		return copier.GetError();
	}
	else
	{
		ULONG64 nSrcChildData = 0;

		int nError = rSrcFile->Open(GENERIC_READ, 0, OPEN_EXISTING, 0, &nSrcChildData);

		if (nError != 0)
			return nError;

		ULONG64 nDstChildData = 0;

		nError = rDstFile->Open(GENERIC_WRITE, 0, OPEN_EXISTING, 0, &nDstChildData);

		if (nError != 0)
		{
			rSrcFile->Close(&nSrcChildData);

			return nError;
		}


		char sBuffer[1024*4]; //Copy in 4K blocks.

		LONGLONG nPos = 0;
		
		DWORD nRead = 0;

		do
		{
			nError = rSrcFile->Read(sBuffer, 1024*4, &nRead, nPos, &nSrcChildData);
			
			if (nRead == 0)
				break;

			DWORD nWritten;

			nError = rDstFile->Write(sBuffer, nRead, &nWritten, nPos, &nDstChildData);

			if (nWritten != nRead)
			{
				if (nError == 0)
					nError = -1; //There is something wrong, so report that

				break;
			}

			nPos += nRead;
		}
		while(nRead != 0);

		if (nError == ERROR_HANDLE_EOF)
			nError = 0;

		rDstFile->SetEndOfFile(nPos, &nDstChildData);
		rSrcFile->Close(&nSrcChildData);

		//Restore any readonly attribute
		rDstFile->SetAttributes(rDstFile->GetAttributes()|(rSrcFile->GetAttributes()&FILE_ATTRIBUTE_READONLY), &nDstChildData);

		rDstFile->Close(&nDstChildData);

		return nError;
	}
}



bool FolderCopier::Do(LPCWSTR sName, IPtrPlisgoFSFile file)
{
	IPtrPlisgoFSFile dstFile;

	//Create with same attributes, bar readonly because we are going to write to the file
	m_nError = m_pDstFolder->CreateChild(dstFile, sName, file->GetAttributes()&~FILE_ATTRIBUTE_READONLY);

	if (m_nError != 0)
		return false;

	m_nError = CopyPlisgoFile(file, dstFile);

	if (m_nError != 0)
		return false;

	return true;
}



int						PlisgoFSFolder::Repath(LPCWSTR sOldName, LPCWSTR sNewName,
											   bool bReplaceExisting, PlisgoFSFolder* pNewParent)
{
	/*
	Before you get upset, please think about moving, hard. You will see if it goes wrong, you are in trouble.....
	Same disc, easy, you are just changing where the folder is found, really not changing the data much.
	If it NOT THE SAME DISC, where a move is really a copy and delete.
	You will soon start to see a sea of possibilities......
	*/

	if (sOldName == NULL || sNewName == NULL)
		return -ERROR_BAD_PATHNAME;

	if (pNewParent == NULL)
		pNewParent = this;

	IPtrPlisgoFSFile oldChild = GetChild(sOldName);

	if (oldChild.get() == NULL)
		return -ERROR_FILE_NOT_FOUND;

	IPtrPlisgoFSFile newChild = pNewParent->GetChild(sNewName);

	int nError = 0;

	if (newChild.get() != NULL)
	{
		if (bReplaceExisting)
			nError = -ERROR_ALREADY_EXISTS;
		else
			nError = pNewParent->RemoveChild(sNewName);
	}

	if (nError != 0)
		return nError;
	
	nError = pNewParent->AddChild(sNewName, oldChild);

	if (nError != 0)
		return nError;

	nError = RemoveChild(sOldName);

	if (nError != 0)
		pNewParent->RemoveChild(sNewName);

	return 0;
}

/*
****************************************************************************
*/

void				PlisgoFSFileList::AddFile(LPCWSTR sName, IPtrPlisgoFSFile file)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	m_Files[sName] = file;
}
	

void				PlisgoFSFileList::RemoveFile(LPCWSTR sName)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	m_Files.erase(sName);
}


bool				PlisgoFSFileList::ForEachFile(PlisgoFSFolder::EachChild& rEachFile) const
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	for(FileMap::const_iterator it = m_Files.begin();
		it != m_Files.end(); ++it)
	{
		if (!rEachFile.Do(it->first.c_str(), it->second))
			return false;
	}

	return true;
}


IPtrPlisgoFSFile	PlisgoFSFileList::GetFile(LPCWSTR sName) const
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	FileMap::const_iterator it = m_Files.find(sName);
	
	if (it != m_Files.end())
		return it->second;

	return IPtrPlisgoFSFile();
}


IPtrPlisgoFSFile	PlisgoFSFileList::GetFile(UINT nIndex) const
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	if (nIndex >= m_Files.size())
		return IPtrPlisgoFSFile();

	FileMap::const_iterator it = m_Files.begin();

	while(nIndex--)
		++it;

	return it->second;
}


UINT				PlisgoFSFileList::GetLength() const
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	return (UINT)m_Files.size();
}


void				PlisgoFSFileList::Clear()
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	m_Files.clear();
}

/*
****************************************************************************
*/
int		PlisgoFSStorageFolder::CreateChild(IPtrPlisgoFSFile& rChild, LPCWSTR sName, DWORD nAttr)
{
	if (sName == NULL)
		return -ERROR_INVALID_NAME;

	if (nAttr & FILE_ATTRIBUTE_DIRECTORY)
		rChild.reset(new PlisgoFSStorageFolder);
	else
		rChild.reset(new PlisgoFSDataFile);

	return AddChild(sName, rChild);
}

/*
****************************************************************************
*/

PlisgoFSDataFileReadOnly::PlisgoFSDataFileReadOnly(	void*	pData,
													size_t	nDataSize,
													bool	bOwnMemory,
													bool	bFreeNotDelete)
{
	m_pData = (BYTE*)pData;
	m_nDataSize = nDataSize;
	m_bOwnMemory = bOwnMemory;
	m_bFreeNotDelete = bFreeNotDelete;
}


PlisgoFSDataFileReadOnly::~PlisgoFSDataFileReadOnly()
{
	if (m_bOwnMemory)
	{
		if (m_bFreeNotDelete)
			free(m_pData);
		else
			delete m_pData;
	}
}



int		PlisgoFSDataFileReadOnly::Open(	DWORD		nDesiredAccess,
										DWORD		nShareMode,
										DWORD		nCreationDisposition,
										DWORD		nFlagsAndAttributes,
										ULONGLONG*	pInstanceData)
{
	if (nFlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return -ERROR_ACCESS_DENIED;

	int nError = PlisgoFSFile::Open(	nDesiredAccess,
										nShareMode,
										nCreationDisposition,
										nFlagsAndAttributes,
										pInstanceData);
	if (nError != 0)
		return nError;

	bool bWrite = (nDesiredAccess & GENERIC_WRITE || nDesiredAccess & GENERIC_ALL || nDesiredAccess & FILE_WRITE_DATA);

	if (bWrite)
		return -ERROR_ACCESS_DENIED;

	return 0;
}


int		PlisgoFSDataFileReadOnly::Read(	LPVOID		pBuffer,
										DWORD		nNumberOfBytesToRead,
										LPDWORD		pnNumberOfBytesRead,
										LONGLONG	nOffset,
										ULONGLONG*	)
{
	if (m_pData != NULL && nOffset < m_nDataSize)
	{
		const DWORD	nLength	= (DWORD)(m_nDataSize-nOffset);
		const DWORD	nCopySize = min(nLength,nNumberOfBytesToRead);

		memcpy_s(pBuffer, nNumberOfBytesToRead, &m_pData[nOffset], nCopySize);

		*pnNumberOfBytesRead = nCopySize;

		return 0;
	}
	else *pnNumberOfBytesRead = 0;

	return -ERROR_HANDLE_EOF;
}

/*
****************************************************************************
*/
PlisgoFSDataFile::PlisgoFSDataFile(BYTE* pData, size_t nDataSize, bool bReadOnly, bool bCopy)
{
	m_pData = NULL;
	m_nDataSize = m_nDataUsedSize = 0;
	m_bOwnMemory = true;
	m_bWriteOpen = false;
	m_bVolatile = false;

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


PlisgoFSDataFile::PlisgoFSDataFile()
{
	m_pData = NULL;
	m_nDataSize = m_nDataUsedSize = 0;
	m_bOwnMemory = true;
	m_bWriteOpen = false;
	m_bReadOnly = false;
	m_bVolatile = false;
	GetSystemTimeAsFileTime(&m_CreatedTime);
	m_LastWrite = m_CreatedTime;
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


PlisgoFSStringReadOnly::PlisgoFSStringReadOnly(	const std::wstring& rsData)
{
	m_bVolatile = false;
	FromWide(m_sData, rsData);
}

PlisgoFSStringReadOnly::PlisgoFSStringReadOnly(	const std::string& sData)
{
	m_bVolatile = false;
	m_sData		= sData;
}



int				PlisgoFSStringReadOnly::Open(	DWORD		nDesiredAccess,
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

	bool bWrite = (nDesiredAccess & GENERIC_WRITE || nDesiredAccess & GENERIC_ALL || nDesiredAccess & FILE_WRITE_DATA);

	if (bWrite)
		return -ERROR_ACCESS_DENIED;

	*pInstanceData = GENERIC_READ;

	return 0;
}


int				PlisgoFSStringReadOnly::Read(	LPVOID		pBuffer,
												DWORD		nNumberOfBytesToRead,
												LPDWORD		pnNumberOfBytesRead,
												LONGLONG	nOffset,
												ULONGLONG*	)
{
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


int				PlisgoFSStringReadOnly::Close(ULONGLONG* pInstanceData)
{
	assert(pInstanceData != NULL);

	*pInstanceData = 0;

	return 0;
}

/*
****************************************************************************
*/
PlisgoFSStringFile::PlisgoFSStringFile() : PlisgoFSStringReadOnly()
{
	m_bReadOnly = false;
	m_bWriteOpen = false;
}


PlisgoFSStringFile::PlisgoFSStringFile(	const std::wstring& rsData) : PlisgoFSStringReadOnly(rsData)
{
	m_bReadOnly = false;
	m_bWriteOpen = false;
}


PlisgoFSStringFile::PlisgoFSStringFile(	const std::string& rsData) : PlisgoFSStringReadOnly(rsData)
{
	m_bReadOnly = false;
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
									ULONGLONG*	pInstanceData)
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	return PlisgoFSStringReadOnly::Read(pBuffer, nNumberOfBytesToRead, pnNumberOfBytesRead, nOffset, pInstanceData);
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


PlisgoFSRealFile::PlisgoFSRealFile(const std::wstring& rsRealFile)
{
	m_sRealFile = rsRealFile;
}


bool			PlisgoFSRealFile::IsValid() const
{
	return (GetFileAttributesW(m_sRealFile.c_str()) != INVALID_FILE_ATTRIBUTES);
}


DWORD			PlisgoFSRealFile::GetAttributes() const
{
	return GetFileAttributesW(m_sRealFile.c_str());
}


LONGLONG		PlisgoFSRealFile::GetSize() const
{
	WIN32_FILE_ATTRIBUTE_DATA  data = {0};

	GetFileAttributesExW(m_sRealFile.c_str(), GetFileExInfoStandard, &data);

	LARGE_INTEGER temp;

	temp.LowPart = data.nFileSizeLow;
	temp.HighPart = data.nFileSizeHigh;

	return temp.QuadPart;
}


bool			PlisgoFSRealFile::GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const
{
	WIN32_FILE_ATTRIBUTE_DATA  data = {0};

	if (!GetFileAttributesExW(m_sRealFile.c_str(), GetFileExInfoStandard, &data))
		return false;

	rCreation	= data.ftCreationTime;
	rLastAccess	= data.ftLastAccessTime;
	rLastWrite	= data.ftLastWriteTime;

	return true;
}


int				PlisgoFSRealFile::GetHandleInfo(LPBY_HANDLE_FILE_INFORMATION pInfo, ULONGLONG* pInstanceData)
{
	assert(pInstanceData != NULL);

	HANDLE hFile = *(HANDLE*)pInstanceData;

	if(!GetFileInformationByHandle(hFile, pInfo))
		return -(int)GetLastError();

	return 0;
}

int				PlisgoFSRealFile::Open(	DWORD		nDesiredAccess,
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


int				PlisgoFSRealFile::Read(LPVOID		pBuffer,
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


int				PlisgoFSRealFile::Write(	LPCVOID		pBuffer,
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


int				PlisgoFSRealFile::FlushBuffers(ULONGLONG* pInstanceData)
{
	assert(pInstanceData != NULL);

	HANDLE hFile = *(HANDLE*)pInstanceData;

	if (FlushFileBuffers(hFile))
		return 0;
	else
		return -(int)GetLastError();
}


int				PlisgoFSRealFile::SetEndOfFile(LONGLONG nEndPos, ULONGLONG* pInstanceData)
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


int				PlisgoFSRealFile::Close(ULONGLONG* pInstanceData)
{
	assert(pInstanceData != NULL);

	HANDLE hFile = *(HANDLE*)pInstanceData;

	if (CloseHandle(hFile))
		return 0;
	else
		return -(int)GetLastError();
}




int				PlisgoFSRealFile::LockFile(LONGLONG	nByteOffset,
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


int				PlisgoFSRealFile::UnlockFile(	LONGLONG	nByteOffset,
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


int				PlisgoFSRealFile::SetFileTimes(const FILETIME* pCreation,
												const FILETIME* pLastAccess,
												const FILETIME* pLastWrite,
												ULONGLONG*		pInstanceData)
{
	HANDLE hFile = *(HANDLE*)pInstanceData;

	if(!::SetFileTime(hFile, pCreation, pLastAccess, pLastWrite))
		return -(int)GetLastError();

	return 0;
}


int				PlisgoFSRealFile::SetAttributes(DWORD	nFileAttributes, ULONGLONG* )
{
	if (!::SetFileAttributesW(m_sRealFile.c_str(), nFileAttributes))
		return -(int)GetLastError();

	return 0;
}

/*
****************************************************************************
*/


PlisgoFSRealFolder::PlisgoFSRealFolder(const std::wstring& rsRealFolder) 
{
	m_sRealPath = rsRealFolder;

	boost::trim_right_if(m_sRealPath, boost::is_any_of(L"\\"));
}


bool			PlisgoFSRealFolder::IsValid() const
{
	return (GetFileAttributesW(m_sRealPath.c_str()) != INVALID_FILE_ATTRIBUTES);
}


DWORD			PlisgoFSRealFolder::GetAttributes() const
{
	return GetFileAttributesW(m_sRealPath.c_str());
}


bool			PlisgoFSRealFolder::GetFileTimes(FILETIME& rCreation, FILETIME& rLastAccess, FILETIME& rLastWrite) const
{
	WIN32_FILE_ATTRIBUTE_DATA  data = {0};

	if (!GetFileAttributesExW(m_sRealPath.c_str(), GetFileExInfoStandard, &data))
		return false;

	rCreation	= data.ftCreationTime;
	rLastAccess	= data.ftLastAccessTime;
	rLastWrite	= data.ftLastWriteTime;

	return true;
}


int				PlisgoFSRealFolder::SetFileTimes(const FILETIME* pCreation,
												const FILETIME* pLastAccess,
												const FILETIME* pLastWrite,
												ULONGLONG*		pInstanceData)
{
	HANDLE hHandle = CreateFileW(	m_sRealPath.c_str(),
									GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
									NULL, OPEN_EXISTING,
									FILE_FLAG_BACKUP_SEMANTICS, NULL); 

	if (hHandle == NULL || hHandle == INVALID_HANDLE_VALUE)
		return -(int)GetLastError();

	int nError = 0;

	if(!::SetFileTime(hHandle, pCreation, pLastAccess, pLastWrite))
		nError = -(int)GetLastError();

	CloseHandle(hHandle);

	return nError;
}


int					PlisgoFSRealFolder::SetAttributes(DWORD	nFileAttributes, ULONGLONG* )
{
	if (!::SetFileAttributesW(m_sRealPath.c_str(), nFileAttributes))
		return -(int)GetLastError();

	return 0;
}


IPtrPlisgoFSFile	PlisgoFSRealFolder::GetChild(	WIN32_FIND_DATAW& rFind) const
{
	std::wstring sExtra = L"\\";
	sExtra+= rFind.cFileName;

	if (rFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		return IPtrPlisgoFSFile(new PlisgoFSRealFolder(m_sRealPath + sExtra));
	}
	else
	{
		return IPtrPlisgoFSFile(new PlisgoFSRealFile(m_sRealPath + sExtra));
	}
}


bool				PlisgoFSRealFolder::ForEachChild(PlisgoFSFolder::EachChild& rEachChild) const
{
	std::wstring sRealSearchPath = m_sRealPath + L"\\*";

	WIN32_FIND_DATAW	findData;

	HANDLE hFind = FindFirstFileW(sRealSearchPath.c_str(), &findData);

	if (hFind == NULL || hFind == INVALID_HANDLE_VALUE)
		return false;
	
	bool bResult = true;

	if (FindNextFileW(hFind, &findData)) //Skip . and ..
	{
		while(FindNextFileW(hFind, &findData)) 
		{
			IPtrPlisgoFSFile file(GetChild(findData));

			if (!rEachChild.Do(findData.cFileName, file))
			{
				bResult = false;
				break;
			}
		}
	}

	FindClose(hFind);
	
	return bResult;
}


IPtrPlisgoFSFile	PlisgoFSRealFolder::GetChild(LPCWSTR sName) const
{
	assert(sName != NULL);

	std::wstring sRealSearchPath = m_sRealPath + L"\\" + sName;

	WIN32_FIND_DATAW	findData;

	HANDLE hFind = FindFirstFileW(sRealSearchPath.c_str(), &findData);

	if (hFind == NULL || hFind == INVALID_HANDLE_VALUE)
		return IPtrPlisgoFSFile();

	IPtrPlisgoFSFile result = GetChild(findData);

	FindClose(hFind);
	
	return result;
}


int					PlisgoFSRealFolder::AddChild(LPCWSTR sName, IPtrPlisgoFSFile file)
{
	FolderCopier copier(this);

	copier.Do(sName, file);

	return copier.GetError();
}


int					PlisgoFSRealFolder::CreateChild(IPtrPlisgoFSFile& rChild, LPCWSTR sName, DWORD nAttr)
{
	std::wstring sPath = m_sRealPath;
	sPath += L"\\";
	sPath += sName;

	{
		WIN32_FIND_DATAW	findData;

		HANDLE hFind = FindFirstFileW(sPath.c_str(), &findData);

		if (hFind != NULL && hFind != INVALID_HANDLE_VALUE)
		{
			FindClose(hFind);

			return -ERROR_FILE_EXISTS;
		}
	}

	if (nAttr & FILE_ATTRIBUTE_DIRECTORY)
	{
		if (!CreateDirectoryW(sPath.c_str(), NULL))
			return -(int)GetLastError();

		rChild.reset(new PlisgoFSRealFolder(sPath));
	}
	else
	{
		HANDLE hHandle = CreateFileW(	sPath.c_str(),
										GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
										NULL, CREATE_NEW,
										nAttr, NULL); 

		if (hHandle == NULL || hHandle == INVALID_HANDLE_VALUE)
			return -(int)GetLastError();

		CloseHandle(hHandle);

		rChild.reset(new PlisgoFSRealFile(sPath));
	}

	return 0;
}



static int			GetDeleteFolderError(const std::wstring& rsFolder)
{
	WIN32_FIND_DATAW	findData;

	std::wstring sRealSearchPath = rsFolder + L"\\*";

	HANDLE hFind = FindFirstFileW(sRealSearchPath.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		int nError = GetLastError();

		if (nError == ERROR_NO_MORE_FILES)
			return 0;
		else
			return nError;
	}

	//skip .. as well as .
	FindNextFileW(hFind, &findData);

	int nError = 0;

	if (FindNextFileW(hFind, &findData))
		nError = -ERROR_DIR_NOT_EMPTY;

	FindClose(hFind);
	
	return nError;
}


int					PlisgoFSRealFolder::GetDeleteError(ULONGLONG* pInstanceData) const
{
	return GetDeleteFolderError(m_sRealPath);
}


int					PlisgoFSRealFolder::GetRemoveChildError(LPCWSTR sName) const
{
	std::wstring sRealPath = m_sRealPath + L"\\" + sName;

	DWORD nAttr = GetFileAttributesW(sRealPath.c_str());

	if (nAttr == INVALID_FILE_ATTRIBUTES)
		return -ERROR_FILE_NOT_FOUND;
	
	if (nAttr&FILE_ATTRIBUTE_DIRECTORY)
		return GetDeleteFolderError(sRealPath);
	else
		return 0;
}


int					PlisgoFSRealFolder::RemoveChild(LPCWSTR sName)
{
	std::wstring sRealPath = m_sRealPath + L"\\" + sName;

	DWORD nAttr = GetFileAttributesW(sRealPath.c_str());

	if (nAttr == INVALID_FILE_ATTRIBUTES)
		return 0; //Already gone
	
	BOOL bSuccess;

	if (nAttr&FILE_ATTRIBUTE_DIRECTORY)
		bSuccess = RemoveDirectoryW(sRealPath.c_str());
	else
		bSuccess = DeleteFileW(sRealPath.c_str());

	if (!bSuccess)
		return -(int)GetLastError();

	return 0;
}


int					PlisgoFSRealFolder::Repath(	LPCWSTR sOldName, LPCWSTR sNewName,
														bool bReplaceExisting, PlisgoFSFolder* pNewParent)
{
	if (sOldName == NULL || sNewName == NULL)
		return -ERROR_BAD_PATHNAME;

	PlisgoFSRealFolder* pNewParent2 = dynamic_cast<PlisgoFSRealFolder*>(pNewParent);

	if (pNewParent2 == NULL && pNewParent != NULL)
		return PlisgoFSFolder::Repath(sOldName, sNewName, bReplaceExisting, pNewParent);

	//Ooooo we can do a proper move!

	std::wstring sOldPath = m_sRealPath;
	std::wstring sNewPath = (pNewParent2 != NULL)?pNewParent2->GetRealPath():m_sRealPath;

	sOldPath += L"\\";
	sOldPath += sOldName;

	sNewPath += L"\\";
	sNewPath += sNewName;

	if (MoveFileExW(sOldPath.c_str(), sNewPath.c_str(), (bReplaceExisting)?MOVEFILE_REPLACE_EXISTING:0))
		return 0;
	else
		return -(int)GetLastError();
}
