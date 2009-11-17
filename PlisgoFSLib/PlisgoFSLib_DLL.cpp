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
#include "PlisgoFSHiddenFolders.h"
#include "PlisgoFSLib_DLL.h"




struct PlisgoFiles
{
	PlisgoFSCachedFileTree					VirtualFiles;
};



struct PlisgoVirtualFile
{
	IPtrPlisgoFSFile file;
	ULONGLONG		 nInstanceData;
};



class CallForEachChild : public PlisgoFSFolder::EachChild
{
public:

	CallForEachChild(PlisgoVirtualFileChildCB cb,  void* pData)
	{
		m_cb		= cb;
		m_pData		= pData;
		m_nError	= 0;

	}

	virtual bool Do(IPtrPlisgoFSFile file)
	{
		FILETIME Creation;
		FILETIME LastAccess;
		FILETIME LastWrite;

		m_nError = file->GetFileTimes(Creation, LastAccess, LastWrite);

		if (m_nError != 0)
			return false;

		m_nError = m_cb(file->GetName(),
						file->GetAttributes(),
						file->GetSize(),
						*(ULONGLONG*)&Creation,
						*(ULONGLONG*)&LastAccess,
						*(ULONGLONG*)&LastWrite,
						m_pData);

		return (m_nError == 0);
	}


	int	GetError() const { return m_nError; }


private:
	PlisgoVirtualFileChildCB	m_cb;
	void*						m_pData;
	int							m_nError;
};




int		PlisgoFilesCreate(PlisgoFiles** ppPlisgoFiles, const WCHAR* sFSName)
{
	if (ppPlisgoFiles == NULL)
		return -ERROR_BAD_ARGUMENTS;

	*ppPlisgoFiles = new PlisgoFiles;

	if (*ppPlisgoFiles == NULL)
		return -ERROR_NO_SYSTEM_RESOURCES;


	RootPlisgoFSFolder* pRoot = new RootPlisgoFSFolder(sFSName);

	if (pRoot == NULL)
	{
		delete *ppPlisgoFiles;
		*ppPlisgoFiles = NULL;

		return -ERROR_NO_SYSTEM_RESOURCES;
	}

	(*ppPlisgoFiles)->VirtualFiles.AddFile(IPtrPlisgoFSFile(pRoot));
	(*ppPlisgoFiles)->VirtualFiles.AddFile(GetPlisgoDesktopIniFile());


	return 0;
}


int		PlisgoFilesDestroy(PlisgoFiles* pPlisgoFiles)
{
	if (pPlisgoFiles == NULL)
		return -ERROR_BAD_ARGUMENTS;

	delete pPlisgoFiles;

	return 0;
}


int		PlisgoFilesForRootFiles(PlisgoFiles* pPlisgoFiles, PlisgoVirtualFileChildCB cb, void* pData)
{
	if (pPlisgoFiles == NULL || cb == NULL)
		return -ERROR_BAD_ARGUMENTS;

	CallForEachChild cbObj(cb, pData);

	pPlisgoFiles->VirtualFiles.ForEachFile(cbObj);

	return cbObj.GetError();
}


int		PlisgoVirtualFileOpen(	PlisgoFiles*		pPlisgoFiles,
								const WCHAR*		sPath,
								BOOL*				pbRootVirtualPath,
								PlisgoVirtualFile**	ppFile,
								DWORD				nDesiredAccess,
								DWORD				nShareMode,
								DWORD				nCreationDisposition,
								DWORD				nFlagsAndAttributes)
{
	if (pPlisgoFiles == NULL || sPath == NULL || ppFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	IPtrPlisgoFSFile file = pPlisgoFiles->VirtualFiles.TracePath(sPath, (bool*)pbRootVirtualPath);

	if (file.get() == NULL)
		return -ERROR_BAD_PATHNAME;


	ULONGLONG		 nInstanceData;

	int nError = file->Open(nDesiredAccess, nShareMode, nCreationDisposition, nFlagsAndAttributes, &nInstanceData);

	if (nError != 0)
		return nError;

	*ppFile = new PlisgoVirtualFile;

	if (*ppFile == NULL)
	{
		nError = file->Close(&nInstanceData);

		if (nError != 0)
			return nError;

		return -ERROR_NO_SYSTEM_RESOURCES;
	}

	(*ppFile)->file				= file;
	(*ppFile)->nInstanceData	= nInstanceData;

	return 0;
}



int		PlisgoVirtualFileClose(PlisgoVirtualFile* pFile)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	int nError = pFile->file->Close(&pFile->nInstanceData);

	if (nError != 0)
		return nError;

	delete pFile;

	return 0;
}



int		PlisgoVirtualFileGetAttributes(PlisgoVirtualFile* pFile, DWORD* pnAttr)
{
	if (pFile == NULL || pnAttr == NULL)
		return -ERROR_BAD_ARGUMENTS;

	*pnAttr = pFile->file->GetAttributes();

	return 0;
}


int		PlisgoVirtualFileGetTimes(	PlisgoVirtualFile*	pFile,
									ULONGLONG*			pnCreation,
									ULONGLONG*			pnLastAccess,
									ULONGLONG*			pnLastWrite)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	FILETIME Creation;
	FILETIME LastAccess;
	FILETIME LastWrite;

	int nError = pFile->file->GetFileTimes(Creation, LastAccess, LastWrite);

	if (nError != 0)
		return nError;

	if (pnCreation != NULL)
		*pnCreation = *(ULONGLONG*)&Creation;

	if (pnLastAccess != NULL)
		*pnLastAccess = *(ULONGLONG*)&LastAccess;

	if (pnLastWrite != NULL)
		*pnLastWrite = *(ULONGLONG*)&LastWrite;

	return 0;
}


int		PlisgoVirtualFileGetSize(PlisgoVirtualFile* pFile, ULONGLONG* pnSize)
{
	if (pFile == NULL || pnSize == NULL)
		return -ERROR_BAD_ARGUMENTS;

	*pnSize = pFile->file->GetSize();

	return 0;
}


int		PlisgoVirtualFileRead(	PlisgoVirtualFile*	pFile,
								LPVOID				pBuffer,
								DWORD				nNumberOfBytesToRead,
								LPDWORD				pnNumberOfBytesRead,
								LONGLONG			nOffset)
{
	if (pFile == NULL || pBuffer == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->Read(pBuffer, nNumberOfBytesToRead, pnNumberOfBytesRead, nOffset, &pFile->nInstanceData);
}


int		PlisgoVirtualFileWrite(	PlisgoVirtualFile*	pFile,
								LPCVOID				pBuffer,
								DWORD				nNumberOfBytesToWrite,
								LPDWORD				pnNumberOfBytesWritten,
								LONGLONG			nOffset)
{
	if (pFile == NULL || pBuffer == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->Write(pBuffer, nNumberOfBytesToWrite, pnNumberOfBytesWritten, nOffset, &pFile->nInstanceData);
}


int		PlisgoVirtualFileLock(PlisgoVirtualFile* pFile, LONGLONG nByteOffset, LONGLONG nByteLength)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->LockFile( nByteOffset, nByteOffset, &pFile->nInstanceData);
}


int		PlisgoVirtualFileUnlock(PlisgoVirtualFile* pFile, LONGLONG nByteOffset, LONGLONG nByteLength)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->UnlockFile( nByteOffset, nByteOffset, &pFile->nInstanceData);
}


int		PlisgoVirtualFileFlushBuffers(PlisgoVirtualFile* pFile)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->FlushBuffers( &pFile->nInstanceData);
}


int		PlisgoVirtualFileFlushSetEndOf(PlisgoVirtualFile* pFile, LONGLONG nEndPos)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->SetEndOfFile(nEndPos, &pFile->nInstanceData);
}


int		PlisgoVirtualFileFlushSetTimes(PlisgoVirtualFile* pFile, ULONGLONG* pnCreation, ULONGLONG* pnLastAccess, ULONGLONG* pnLastWrite)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->SetFileTimes((FILETIME*)pnCreation, (FILETIME*)pnLastAccess, (FILETIME*)pnLastWrite, &pFile->nInstanceData);
}


int		PlisgoVirtualFileFlushSetAttributes(PlisgoVirtualFile* pFile, DWORD nAttr)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->SetFileAttributesW(nAttr, &pFile->nInstanceData);
}


int		PlisgoVirtualFileForEachChild(PlisgoVirtualFile* pFile, PlisgoVirtualFileChildCB cb, void* pData)
{
	if (pFile == NULL || cb == NULL)
		return -ERROR_BAD_ARGUMENTS;

	PlisgoFSFolder* pFolder = pFile->file->GetAsFolder();

	if (pFolder == NULL)
		return -ERROR_ACCESS_DENIED; //Not a folder

	CallForEachChild cbObj(cb, pData);

	pFolder->ForEachChild(cbObj);

	return cbObj.GetError();
}
