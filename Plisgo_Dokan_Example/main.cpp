/*
* Copyright (c) 2009, Eurocom Entertainment Software
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 	* Redistributions of source code must retain the above copyright
* 	  notice, this list of conditions and the following disclaimer.
* 	* Redistributions in binary form must reproduce the above copyright
* 	  notice, this list of conditions and the following disclaimer in the
* 	  documentation and/or other materials provided with the distribution.
* 	* Neither the name of the Eurocom Entertainment Software nor the
*	  names of its contributors may be used to endorse or promote products
* 	  derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY EUROCOM ENTERTAINMENT SOFTWARE ''AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* =============================================================================
*
* This is part of a Plisgo/Dokan example.
* Written by Joe Burmeister (plisgo@eurocom.co.uk)
*/

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <gdiplus.h>
#include "dokan.h"
#include "PlisgoFSHiddenFolders.h"
#include "resource.h"
#include "ProcessesFolder.h"


boost::shared_ptr<ProcessesFolder>			g_ProcessesFolder;
boost::shared_ptr<PlisgoFSCachedFileTree>	g_PlisgoFSFileTree;


//Uncomment if you wish to compile against the latest Dokan from SVN
//#define SVN_DOKAN_VERSION


class DokanPerFileCall : public PlisgoFSFolder::EachChild
{
public:
	DokanPerFileCall(PFillFindData	FillFindData, PDOKAN_FILE_INFO pDokanInfo) :m_FillFindData(FillFindData),
																				m_pDokanInfo(pDokanInfo)
	{
		m_nError		= 0;
	}

	virtual bool Do(IPtrPlisgoFSFile file)
	{
		if (file.get() == NULL)
			return false;

		WIN32_FIND_DATAW data = {0};

		m_nError = file->GetFindFileData(&data);

		if (m_nError != 0)
			return false;

		m_nError = m_FillFindData(&data, m_pDokanInfo);

		return (m_nError == 0);
	}

	int	GetError() const	{ return m_nError; }

private:
	const PFillFindData		m_FillFindData;
	const PDOKAN_FILE_INFO	m_pDokanInfo;
	int						m_nError;
};



static IPtrPlisgoFSFile	TracePath(LPCWSTR	sPath)
{
	return g_PlisgoFSFileTree->TracePath(sPath);
}




int __stdcall	PlisgoExampleCreateFile(LPCWSTR					sFileName,
										DWORD					nAccessMode,
										DWORD					nShareMode,
										DWORD					nCreationDisposition,
										DWORD					nFlagsAndAttributes,
										PDOKAN_FILE_INFO		pDokanFileInfo)
{
	if (sFileName[0] == L'\\' && sFileName[1] == L'\0')
		return 0;

	IPtrPlisgoFSFile file = TracePath(sFileName);

	if (file.get() == NULL)
		return -ERROR_FILE_NOT_FOUND;

	return file->Open(nAccessMode, nShareMode, nCreationDisposition, nFlagsAndAttributes, &pDokanFileInfo->Context);
}


int __stdcall	PlisgoExampleCreateDirectory(	LPCWSTR					sFileName,
												PDOKAN_FILE_INFO		pDokanFileInfo)
{
	return -ERROR_ACCESS_DENIED;
}


int __stdcall	PlisgoExampleOpenDirectory(	LPCWSTR					sFileName,
											PDOKAN_FILE_INFO		pDokanFileInfo)
{
	return PlisgoExampleCreateFile(	sFileName,
									GENERIC_READ,
									FILE_SHARE_READ|FILE_SHARE_WRITE,
									OPEN_EXISTING,
									FILE_FLAG_BACKUP_SEMANTICS,
									pDokanFileInfo);
}


int __stdcall	PlisgoExampleCloseFile(	LPCWSTR					sFileName,
										PDOKAN_FILE_INFO		pDokanFileInfo)
{
	if (sFileName[0] == L'\\' && sFileName[1] == L'\0')
		return 0;

	IPtrPlisgoFSFile file = TracePath(sFileName);

	if (file.get() == NULL)
		return -ERROR_FILE_NOT_FOUND;

	return file->Close(&pDokanFileInfo->Context);
}


int __stdcall	PlisgoExampleReadFile(	LPCWSTR				sFileName,
										LPVOID				pBuffer,
										DWORD				nBufferLength,
										LPDWORD				pnReadLength,
										LONGLONG			nOffset,
										PDOKAN_FILE_INFO	pDokanFileInfo)
{
	IPtrPlisgoFSFile file = TracePath(sFileName);

	if (file.get() == NULL)
		return -ERROR_FILE_NOT_FOUND;

	return file->Read(pBuffer, nBufferLength, pnReadLength, nOffset, &pDokanFileInfo->Context);
}


int __stdcall	PlisgoExampleWriteFile(	LPCWSTR				sFileName,
										LPCVOID				pBuffer,
										DWORD				nNumberOfBytesToWrite,
										LPDWORD				pnNumberOfBytesWritten,
										LONGLONG			nOffset,
										PDOKAN_FILE_INFO	pDokanFileInfo)
{
	IPtrPlisgoFSFile file = TracePath(sFileName);

	if (file.get() == NULL)
		return -ERROR_FILE_NOT_FOUND;

	return file->Write(pBuffer, nNumberOfBytesToWrite, pnNumberOfBytesWritten, nOffset, &pDokanFileInfo->Context);
}


int __stdcall	PlisgoExampleFlushFileBuffers(	LPCWSTR				sFileName,
												PDOKAN_FILE_INFO	pDokanFileInfo)
{
	IPtrPlisgoFSFile file = TracePath(sFileName);

	if (file.get() == NULL)
		return -ERROR_FILE_NOT_FOUND;

	return file->FlushBuffers(&pDokanFileInfo->Context);
}


int __stdcall	PlisgoExampleGetFileInformation(LPCWSTR							sFileName,
												LPBY_HANDLE_FILE_INFORMATION	pHandleFileInformation,
												PDOKAN_FILE_INFO				pDokanFileInfo)
{
	if (sFileName[0] == L'\\' && sFileName[1] == L'\0')
	{
		ZeroMemory(pHandleFileInformation, sizeof(BY_HANDLE_FILE_INFORMATION));

		pHandleFileInformation->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

		GetSystemTimeAsFileTime(&pHandleFileInformation->ftCreationTime);

		pHandleFileInformation->ftLastAccessTime = pHandleFileInformation->ftLastWriteTime = pHandleFileInformation->ftCreationTime;

		pHandleFileInformation->nNumberOfLinks = 1;

		return 0;
	}

	IPtrPlisgoFSFile file = TracePath(sFileName);

	if (file.get() == NULL)
		return -ERROR_FILE_NOT_FOUND;

	return file->GetHandleInfo(pHandleFileInformation, &pDokanFileInfo->Context);
}


int __stdcall	PlisgoExampleFindFiles(	LPCWSTR				sFileName,
										PFillFindData		FillFindData,
										PDOKAN_FILE_INFO	pDokanFileInfo)
{
	WIN32_FIND_DATAW data = {0};

	wcscpy_s(data.cFileName, MAX_PATH, L".");

	data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

	GetSystemTimeAsFileTime(&data.ftCreationTime);

	data.ftLastAccessTime = data.ftLastWriteTime = data.ftCreationTime;

	int nError = FillFindData(&data, pDokanFileInfo);

	if (nError != 0)
		return nError;

	wcscpy_s(data.cFileName, MAX_PATH, L"..");

	nError = FillFindData(&data, pDokanFileInfo);

	if (nError != 0)
		return nError;


	if (sFileName[0] == L'\\' && sFileName[1] == L'\0')
	{
		wcscpy_s(data.cFileName, MAX_PATH, L"processes");

		data.dwFileAttributes	= g_ProcessesFolder->GetAttributes();

		g_ProcessesFolder->GetFileTimes(data.ftCreationTime, data.ftLastAccessTime, data.ftLastWriteTime);

		return FillFindData(&data, pDokanFileInfo);
	}
	
	IPtrPlisgoFSFile file = TracePath(sFileName);

	if (file.get() == NULL)
		return -ERROR_ACCESS_DENIED;

	PlisgoFSFolder* pAsFolder = file->GetAsFolder();

	if (pAsFolder == NULL)
		return -ERROR_ACCESS_DENIED;

	DokanPerFileCall dokanPerFileCall(FillFindData, pDokanFileInfo);

	pAsFolder->ForEachChild(dokanPerFileCall);

	return dokanPerFileCall.GetError();
}


int __stdcall	PlisgoExampleLockFile(	LPCWSTR				sFileName,
										LONGLONG			nByteOffset,
										LONGLONG			nLength,
										PDOKAN_FILE_INFO	pDokanFileInfo)
{
	IPtrPlisgoFSFile file = TracePath(sFileName);

	if (file.get() == NULL)
		return -ERROR_FILE_NOT_FOUND;

	return file->LockFile(nByteOffset, nLength, &pDokanFileInfo->Context);
}


int __stdcall	PlisgoExampleUnlockFile(LPCWSTR				sFileName,
										LONGLONG			nByteOffset,
										LONGLONG			nLength,
										PDOKAN_FILE_INFO	pDokanFileInfo)
{
	IPtrPlisgoFSFile file = TracePath(sFileName);

	if (file.get() == NULL)
		return -ERROR_FILE_NOT_FOUND;

	return file->UnlockFile(nByteOffset, nLength, &pDokanFileInfo->Context);
}


int __stdcall	PlisgoExampleSetEndOfFile(	LPCWSTR				sFileName,
											LONGLONG			nLength,
											PDOKAN_FILE_INFO	pDokanFileInfo)

{
	IPtrPlisgoFSFile file = TracePath(sFileName);

	if (file.get() == NULL)
		return -ERROR_FILE_NOT_FOUND;

	return file->SetEndOfFile(nLength, &pDokanFileInfo->Context);
}


int __stdcall	PlisgoExampleGetVolumeInformation(	LPWSTR				sVolumeNameBuffer,
													DWORD				nVolumeNameSize,
													LPDWORD				pnVolumeSerialNumber,
													LPDWORD				pnMaximumComponentLength,
													LPDWORD				pnFileSystemFlags,
													LPWSTR				sFileSystemNameBuffer,
													DWORD				nFileSystemNameSize,
													PDOKAN_FILE_INFO	pDokanFileInfo)
{
	nVolumeNameSize /= sizeof(WCHAR);
	nFileSystemNameSize /= sizeof(WCHAR);

	wcscpy_s(sFileSystemNameBuffer, nFileSystemNameSize, L"Dokan");
	wcscpy_s(sVolumeNameBuffer, nVolumeNameSize, L"ProcessFS");

	*pnVolumeSerialNumber		= 0x19831116;
	*pnFileSystemFlags			= FILE_UNICODE_ON_DISK|FILE_CASE_PRESERVED_NAMES;
	*pnMaximumComponentLength	= MAX_PATH;

	return 0;
}


int __stdcall	PlisgoExampleUnmount(PDOKAN_FILE_INFO	pDokanFileInfo)
{
	return 0;
}







class GDIOpenClose
{
public:

	GDIOpenClose()		{ Gdiplus::GdiplusStartup(&m_nDiplusToken, &m_GDiplusStartupInput, NULL); }
	~GDIOpenClose()		{ Gdiplus::GdiplusShutdown(m_nDiplusToken); }

	Gdiplus::GdiplusStartupInput	m_GDiplusStartupInput;
	ULONG_PTR						m_nDiplusToken;
};





int __cdecl
main(ULONG argc, PCHAR argv[])
{
	GDIOpenClose					gdi;

	g_ProcessesFolder = boost::make_shared<ProcessesFolder>();

	assert(g_ProcessesFolder.get() != NULL);

	g_PlisgoFSFileTree = boost::make_shared<PlisgoFSCachedFileTree>();

	assert(g_PlisgoFSFileTree.get() != NULL);

	g_PlisgoFSFileTree->AddFile(g_ProcessesFolder);

	DOKAN_OPERATIONS	dokanOperations = {	PlisgoExampleCreateFile,
											PlisgoExampleOpenDirectory,
											PlisgoExampleCreateDirectory,
											NULL,
											PlisgoExampleCloseFile,
											PlisgoExampleReadFile,
											PlisgoExampleWriteFile,
											PlisgoExampleFlushFileBuffers,
											PlisgoExampleGetFileInformation,
											PlisgoExampleFindFiles,
											NULL, // FindFilesWithPattern
											NULL, // SetFileAttributes
											NULL, // SetFileTime
											NULL, // DeleteFile
											NULL, // DeleteDirectory
											NULL, // MoveFile
											PlisgoExampleSetEndOfFile,
#ifdef SVN_DOKAN_VERSION
											NULL, 
#endif//SVN_DOKAN_VERSION
											PlisgoExampleLockFile,
											PlisgoExampleUnlockFile,
											NULL, // GetDiskFreeSpace
											PlisgoExampleGetVolumeInformation,
											PlisgoExampleUnmount };

	DOKAN_OPTIONS dokanOptions;

	ZeroMemory(&dokanOptions, sizeof(DOKAN_OPTIONS));

	dokanOptions.DriveLetter = L'X';

	int status = DokanMain(&dokanOptions, &dokanOperations);

	g_ProcessesFolder.reset();

	return 0;
}

