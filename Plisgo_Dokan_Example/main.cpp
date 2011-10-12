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

#pragma warning(disable:4996)

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
	/*
		Check the path is acturally a folder.
		This will put the file into the cache, so the next call will be quicker, so not much cost.
	*/
	{
		IPtrPlisgoFSFile file = GetPlisgoVFS(pDokanFileInfo)->TracePath(sFileName);
		
		if (file.get() == NULL)
			return -ERROR_PATH_NOT_FOUND;

		if (file->GetAsFolder() == NULL)
			return -ERROR_NO_MORE_ITEMS; //STATUS_NOT_A_DIRECTORY   0xC0000103
	}

	return PlisgoExampleCreateFile(	sFileName,
									GENERIC_READ,
									FILE_SHARE_READ|FILE_SHARE_WRITE,
									OPEN_EXISTING,
									FILE_FLAG_BACKUP_SEMANTICS,
									pDokanFileInfo);
}

/*
	DOKAN does not hide the Cleanup/Close system in Windows.
	Briefly:
	Cleanup is when all the applications (user land) has finished with the file.
	Close is when the OS has finished with the file. In the time between the OS might do reads or writes.
	You can fudge it and use just one or the other, but it will catch you.
	Read up on IRP_MJ_CLEANUP and IRP_MJ_CLOSE go into the MS screaming room.

	In this case PlisgoExampleCloseFile is used for both, the consequence is in PlisgoExampleReadFile and PlisgoExampleWriteFile
*/

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
	PlisgoVFS::PlisgoFileHandle&	rHandle	= (PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context;
	PlisgoVFS*						pVFS	= GetPlisgoVFS(pDokanFileInfo);

	if (rHandle == 0)
	{
		//Ok so this has come in after cleanup and before close. We need to reopen it
		int nError = pVFS->Open(rHandle, sFileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, 0);

		if (nError != 0)
			return nError;
	}

	return pVFS->Read(rHandle, pBuffer, nBufferLength, pnReadLength, nOffset);
}


int __stdcall	PlisgoExampleWriteFile(	LPCWSTR				sFileName,
										LPCVOID				pBuffer,
										DWORD				nNumberOfBytesToWrite,
										LPDWORD				pnNumberOfBytesWritten,
										LONGLONG			nOffset,
										PDOKAN_FILE_INFO	pDokanFileInfo)
{
	PlisgoVFS::PlisgoFileHandle&	rHandle	= (PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context;
	PlisgoVFS*						pVFS	= GetPlisgoVFS(pDokanFileInfo);

	if (rHandle == 0)
	{
		//Ok so this has come in after cleanup and before close. We need to reopen it..
		int nError = pVFS->Open(rHandle, sFileName, GENERIC_WRITE, FILE_SHARE_WRITE, OPEN_EXISTING, 0);

		if (nError != 0)
			return nError;
	}

	if (pDokanFileInfo->WriteToEndOfFile)
	{
		IPtrPlisgoFSFile file = pVFS->GetPlisgoFSFile(rHandle);

		if (file.get() == NULL)
			return -ERROR_INVALID_HANDLE;

		BY_HANDLE_FILE_INFORMATION info;

		if (pVFS->GetHandleInfo(rHandle, &info) == 0)
		{
			LARGE_INTEGER temp;

			temp.HighPart = info.nFileIndexHigh;
			temp.LowPart = info.nFileSizeLow;

			nOffset = temp.QuadPart;
		}
		else nOffset = file->GetSize();
	}

	return pVFS->Write(rHandle, pBuffer, nNumberOfBytesToWrite, pnNumberOfBytesWritten, nOffset);
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


int __stdcall	PlisgoExampleFindFiles(	LPCWSTR				sFileName,
										PFillFindData		FillFindData,
										PDOKAN_FILE_INFO	pDokanFileInfo)
{
	PlisgoVFS::PlisgoFileHandle& rHandle = (PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context;

	WIN32_FIND_DATAW data = {0};
	int nError;

	PlisgoVFS* pVFS = GetPlisgoVFS(pDokanFileInfo);

	IPtrPlisgoFSFile parent = pVFS->GetParent(rHandle);

	if (parent.get() != NULL)
	{
		IPtrPlisgoFSFile self = pVFS->GetPlisgoFSFile(rHandle);

		self->GetFileInfo(&data);

		wcscpy_s(data.cFileName, MAX_PATH, L".");

		nError = FillFindData(&data, pDokanFileInfo);

		if (nError != 0)
			return nError;

		parent->GetFileInfo(&data);

		wcscpy_s(data.cFileName, MAX_PATH, L"..");

		nError = FillFindData(&data, pDokanFileInfo);

		if (nError != 0)
			return nError;
	}

	PlisgoFSFolder::ChildNames children;

	nError = pVFS->GetChildren(rHandle, children);
	
	if (nError != 0)
		return nError;

	for(PlisgoFSFolder::ChildNames::const_iterator it = children.begin(); it != children.end(); ++it)
	{
		IPtrPlisgoFSFile child;

		if (pVFS->GetChild(child, rHandle, it->c_str()) == 0)
		{
			child->GetFileInfo(&data);

			wcscpy_s(data.cFileName, MAX_PATH, it->c_str());

			nError = FillFindData(&data, pDokanFileInfo);

			if (nError != 0)
				return nError;
		}
	}

	return 0;
}


int __stdcall	PlisgoExampleSetFileAttributes(	LPCWSTR				sFileName,
												DWORD				nFileAttributes,
												PDOKAN_FILE_INFO	pDokanFileInfo)
{
	return GetPlisgoVFS(pDokanFileInfo)->SetAttributes((PlisgoVFS::PlisgoFileHandle&)pDokanFileInfo->Context, nFileAttributes);
}



#define MAKENULL_IF_ZERO(_t)	(_t = (_t != NULL && (*(ULONGLONG*)_t) == 0)?NULL:_t)

int __stdcall	PlisgoExampleSetFileTime(	LPCWSTR				sFileName,
											CONST FILETIME*		pCreationTime,
											CONST FILETIME*		pLastAccessTime,
											CONST FILETIME*		pLastWriteTime,
											PDOKAN_FILE_INFO	pDokanFileInfo)
{
	MAKENULL_IF_ZERO(pCreationTime);
	MAKENULL_IF_ZERO(pLastAccessTime);
	MAKENULL_IF_ZERO(pLastWriteTime);

	if (pCreationTime == NULL && pLastAccessTime == NULL && pLastWriteTime == NULL)
		return 0;

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
											PlisgoExampleCloseFile,
											PlisgoExampleCloseFile,  //read comment above PlisgoExampleCloseFile
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

	dokanOptions.DriveLetter = L'Q';
	dokanOptions.GlobalContext = (ULONG64)vfs.get();
	dokanOptions.Options = DOKAN_OPTION_REMOVABLE;

	int status = DokanMain(&dokanOptions, &dokanOperations);

	return 0;
}

