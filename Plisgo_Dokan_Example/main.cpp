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
#include "../PlisgoFSLib/PlisgoFSHiddenFolders.h"
#include "../PlisgoFSLib/PlisgoVFS.h"
#include "resource.h"
#include "ProcessesFolder.h"



inline PlisgoVFS*	GetPlisgoVFS(PDOKAN_FILE_INFO	pDokanFileInfo)
{
	return (PlisgoVFS*)pDokanFileInfo->DokanOptions->GlobalContext;
}


int __stdcall	PlisgoExampleCreateFile(LPCWSTR					sFileName,
										DWORD					nAccessMode,
										DWORD					nShareMode,
										DWORD					nCreationDisposition,
										DWORD					nFlagsAndAttributes,
										PDOKAN_FILE_INFO		pDokanFileInfo)
{
	return GetPlisgoVFS(pDokanFileInfo)->Open((PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context, sFileName,
							nAccessMode, nShareMode, nCreationDisposition, nFlagsAndAttributes);
}


int __stdcall	PlisgoExampleCreateDirectory(	LPCWSTR					sFileName,
												PDOKAN_FILE_INFO		pDokanFileInfo)
{
	return PlisgoExampleCreateFile(	sFileName,
									GENERIC_READ|GENERIC_WRITE,
									FILE_SHARE_READ|FILE_SHARE_WRITE,
									CREATE_NEW,
									FILE_FLAG_BACKUP_SEMANTICS|FILE_ATTRIBUTE_DIRECTORY,
									pDokanFileInfo);
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
	return GetPlisgoVFS(pDokanFileInfo)->Close((PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context, (pDokanFileInfo->DeleteOnClose)?true:false);
}


int __stdcall	PlisgoExampleReadFile(	LPCWSTR				sFileName,
										LPVOID				pBuffer,
										DWORD				nBufferLength,
										LPDWORD				pnReadLength,
										LONGLONG			nOffset,
										PDOKAN_FILE_INFO	pDokanFileInfo)
{
	return GetPlisgoVFS(pDokanFileInfo)->Read((PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context,
								pBuffer, nBufferLength, pnReadLength, nOffset);
}


int __stdcall	PlisgoExampleWriteFile(	LPCWSTR				sFileName,
										LPCVOID				pBuffer,
										DWORD				nNumberOfBytesToWrite,
										LPDWORD				pnNumberOfBytesWritten,
										LONGLONG			nOffset,
										PDOKAN_FILE_INFO	pDokanFileInfo)
{
	return GetPlisgoVFS(pDokanFileInfo)->Write((PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context,
								pBuffer, nNumberOfBytesToWrite, pnNumberOfBytesWritten, nOffset);
}


int __stdcall	PlisgoExampleFlushFileBuffers(	LPCWSTR				sFileName,
												PDOKAN_FILE_INFO	pDokanFileInfo)
{
	return GetPlisgoVFS(pDokanFileInfo)->FlushBuffers((PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context);
}


int __stdcall	PlisgoExampleGetFileInformation(LPCWSTR							sFileName,
												LPBY_HANDLE_FILE_INFORMATION	pHandleFileInformation,
												PDOKAN_FILE_INFO				pDokanFileInfo)
{
	return GetPlisgoVFS(pDokanFileInfo)->GetHandleInfo((PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context, pHandleFileInformation);
}



class DokanPerFileCall : public PlisgoFSFolder::EachChild
{
public:
	DokanPerFileCall(PFillFindData	FillFindData, PDOKAN_FILE_INFO pDokanInfo) :m_FillFindData(FillFindData),
																				m_pDokanInfo(pDokanInfo)
	{
		m_nError		= 0;
	}

	virtual bool Do(LPCWSTR sName, IPtrPlisgoFSFile file)
	{
		WIN32_FIND_DATAW data = {0};

		if (file.get() == NULL)
		{
			GetSystemTimeAsFileTime(&data.ftCreationTime);

			data.ftLastAccessTime = data.ftLastWriteTime = data.ftCreationTime;
		}
		else file->GetFileInfo(&data);

		wcscpy_s(data.cFileName, MAX_PATH, sName);

		m_nError = m_FillFindData(&data, m_pDokanInfo);

		return (m_nError == 0);
	}

	int	GetError() const	{ return m_nError; }

private:
	const PFillFindData		m_FillFindData;
	const PDOKAN_FILE_INFO	m_pDokanInfo;
	int						m_nError;
};



int __stdcall	PlisgoExampleFindFiles(	LPCWSTR				sFileName,
										PFillFindData		FillFindData,
										PDOKAN_FILE_INFO	pDokanFileInfo)
{
	DokanPerFileCall dokanPerFileCall(FillFindData, pDokanFileInfo);

	return GetPlisgoVFS(pDokanFileInfo)->ForEachChild((PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context, dokanPerFileCall);
}


int __stdcall	PlisgoExampleSetFileAttributes(	LPCWSTR				sFileName,
												DWORD				nFileAttributes,
												PDOKAN_FILE_INFO	pDokanFileInfo)
{
	return GetPlisgoVFS(pDokanFileInfo)->SetAttributes((PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context, nFileAttributes);
}


int __stdcall	PlisgoExampleSetFileTime(	LPCWSTR				sFileName,
											CONST FILETIME*		pCreationTime,
											CONST FILETIME*		pLastAccessTime,
											CONST FILETIME*		pLastWriteTime,
											PDOKAN_FILE_INFO	pDokanFileInfo)
{
	return GetPlisgoVFS(pDokanFileInfo)->SetFileTimes((PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context, pCreationTime, pLastAccessTime, pLastWriteTime);
}


int __stdcall	PlisgoExampleDeleteFile(LPCWSTR				sFileName,
										PDOKAN_FILE_INFO	pDokanFileInfo)
{
	return GetPlisgoVFS(pDokanFileInfo)->GetDeleteError((PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context);
}

int __stdcall	PlisgoExampleDeleteDirectory(	LPCWSTR				sFileName,
												PDOKAN_FILE_INFO	pDokanFileInfo)
{
	return PlisgoExampleDeleteFile(sFileName, pDokanFileInfo);
}


int __stdcall	PlisgoExampleMoveFile(	LPCWSTR				sSrcFileName,
										LPCWSTR				sDstFileName,
										BOOL				bReplaceIfExisting,
										PDOKAN_FILE_INFO	pDokanFileInfo)
{
	PlisgoVFS* pVFS = GetPlisgoVFS(pDokanFileInfo);

	//Ensure it's closed before moving/renaming
	pVFS->Close((PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context);

	return pVFS->Repath(sSrcFileName, sDstFileName, (bReplaceIfExisting == TRUE));
}


int __stdcall	PlisgoExampleLockFile(	LPCWSTR				sFileName,
										LONGLONG			nByteOffset,
										LONGLONG			nLength,
										PDOKAN_FILE_INFO	pDokanFileInfo)
{
	return GetPlisgoVFS(pDokanFileInfo)->LockFile((PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context, nByteOffset, nLength);
}


int __stdcall	PlisgoExampleUnlockFile(LPCWSTR				sFileName,
										LONGLONG			nByteOffset,
										LONGLONG			nLength,
										PDOKAN_FILE_INFO	pDokanFileInfo)
{
	return GetPlisgoVFS(pDokanFileInfo)->UnlockFile((PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context, nByteOffset, nLength);
}



int __stdcall	PlisgoExampleSetEndOfFile(	LPCWSTR				sFileName,
											LONGLONG			nLength,
											PDOKAN_FILE_INFO	pDokanFileInfo)

{
	return GetPlisgoVFS(pDokanFileInfo)->SetEndOfFile((PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context, nLength);
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

	IPtrPlisgoFSFolder root = boost::make_shared<PlisgoFSStorageFolder>();

	IPtrIShellInfoFetcher	shellInfo = boost::make_shared<ProcessesFolderShellInterface>();

	IPtrPlisgoFSFolder processes = boost::make_shared<ProcessesFolder>();

	root->AddChild(L"Processes", processes);

	IPtrPlisgoVFS vfs = boost::make_shared<PlisgoVFS>(root);

	shellInfo->CreatePlisgoFolder(vfs, L"\\Processes");


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
											PlisgoExampleSetFileAttributes,
											PlisgoExampleSetFileTime,
											PlisgoExampleDeleteFile,
											PlisgoExampleDeleteDirectory,
											PlisgoExampleMoveFile,
											PlisgoExampleSetEndOfFile,
											NULL, 
											PlisgoExampleLockFile,
											PlisgoExampleUnlockFile,
											NULL, // GetDiskFreeSpace
											PlisgoExampleGetVolumeInformation,
											PlisgoExampleUnmount };

	DOKAN_OPTIONS dokanOptions;

	ZeroMemory(&dokanOptions, sizeof(DOKAN_OPTIONS));

	dokanOptions.DriveLetter = L'X';
	dokanOptions.GlobalContext = (ULONG64)vfs.get();
	dokanOptions.Options = DOKAN_OPTION_REMOVABLE;

	int status = DokanMain(&dokanOptions, &dokanOperations);

	return 0;
}

