/*
	Copyright 2009 Eurocom Entertainment Software Limited

    This file is part of PlisgoFSLib test programm to check the DLL.

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

#define WIN32_EXTRA_LEAN
#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>
#include <assert.h>

#include "../PlisgoFSLib_DLL.h"
#include "Basic_FS.h"
#include "Basic_FS_UI.h"
#include "Basic_FS_Host.h"


static void PrintPlisgoFileInfo(PlisgoFileInfo* pPlisgoFileInfo, int nDepth)
{
	BOOL bFolder;

	bFolder = ((pPlisgoFileInfo->nAttr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)?TRUE:FALSE;

	while(nDepth--)
		wprintf(L"    ");

	wprintf(L"%s %s: %I64u\n", pPlisgoFileInfo->sName, (bFolder)?L"(folder)":L"", pPlisgoFileInfo->nSize);
}



struct DumpTreePacket
{
	int				nDepth;
	PlisgoFiles*	pPlisgoFiles;
	WCHAR			sPath[MAX_PATH];
};
typedef struct DumpTreePacket DumpTreePacket;


static BOOL DumpVirtualTreeCB(PlisgoFileInfo* pPlisgoFileInfo, void* pData)
{
	DumpTreePacket* pPacket = (DumpTreePacket*)pData;

	assert(pData != NULL && pPlisgoFileInfo != NULL);

	PrintPlisgoFileInfo(pPlisgoFileInfo, pPacket->nDepth);

	if (pPlisgoFileInfo->nAttr & FILE_ATTRIBUTE_DIRECTORY)
	{
		size_t nPos = wcslen(pPacket->sPath);

		PlisgoVirtualFile* pVirtualFile = NULL;

		++pPacket->nDepth;

		wcscat_s(pPacket->sPath, MAX_PATH, L"\\");
		wcscat_s(pPacket->sPath, MAX_PATH, pPlisgoFileInfo->sName);

		PlisgoVirtualFileOpen(pPacket->pPlisgoFiles, pPacket->sPath, &pVirtualFile, GENERIC_READ, 0, OPEN_EXISTING, 0);

		if (pVirtualFile != NULL)
		{
			PlisgoVirtualFileForEachChild(pVirtualFile, DumpVirtualTreeCB, pPacket);

			PlisgoVirtualFileClose(pVirtualFile);
		}

		pPacket->sPath[nPos] = L'\0';


		--pPacket->nDepth;
	}

	return TRUE;
}


static void PrintBasicFile(LPCWSTR sName, BasicFile* pFile, int nDepth)
{
	BasicStats stats;

	while(nDepth--)
		wprintf(L"    ");

	BasicFile_GetStats(pFile, &stats);

	wprintf(L"%s %s: %I64u\n", sName, (BasicFile_IsFolder(pFile))?L"(folder)":L"", stats.nSize);
}



static void CreateTreeFromLocalFolder(BasicFile* pParent, LPCWSTR sLocalFolder)
{
	WIN32_FIND_DATAW	findData;

	HANDLE hFind = FindFirstFileW(sLocalFolder, &findData);

	if (hFind != NULL && hFind != INVALID_HANDLE_VALUE)
	{
		if (FindNextFileW(hFind, &findData)) //Skip . and ..
			while(FindNextFileW(hFind, &findData)) 
			{
				BasicFile* pChild;

				if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					WCHAR sBuffer[MAX_PATH];

					pChild = BasicFolder_Create();

					wcscpy_s(sBuffer, MAX_PATH, sLocalFolder);
					sBuffer[wcslen(sBuffer)-1] = L'\0';
					wcscat_s(sBuffer, MAX_PATH, findData.cFileName);
					wcscat_s(sBuffer, MAX_PATH, L"\\*");

					CreateTreeFromLocalFolder(pChild, sBuffer);
				}
				else pChild = BasicFile_Create();

				BasicFolder_AddChild(pParent, findData.cFileName, pChild);

				BasicFile_Release(pChild);
			}

		FindClose(hFind);
	}
}



static void CreateTestTree(BasicFile* pParent)
{
	WCHAR sBuffer[MAX_PATH];

	GetCurrentDirectory(MAX_PATH, sBuffer);

	wcscat_s(sBuffer, MAX_PATH, L"\\*");

	CreateTreeFromLocalFolder(pParent, sBuffer);


}

/*
	Example of using virtual files along side own files
*/

static BOOL DumpCombinedTreeCB(LPCWSTR sName, BasicFile* pFile, void* pData)
{
	DumpTreePacket* pPacket = (DumpTreePacket*)pData;
	PlisgoVirtualFile* pVirtualFile = NULL;
	size_t nPos;

	assert(pData != NULL && pFile != NULL && sName != NULL);

	nPos = wcslen(pPacket->sPath);

	if (sName[0] != L'\0')
	{
		wcscat_s(pPacket->sPath, MAX_PATH, L"\\");
		wcscat_s(pPacket->sPath, MAX_PATH, sName);
	}
	
	//Does this BasicFile have a virtual override?
	PlisgoVirtualFileOpen(pPacket->pPlisgoFiles, pPacket->sPath, &pVirtualFile, GENERIC_READ, 0, OPEN_EXISTING, 0);

	if (pVirtualFile != NULL)
	{
		//This BasicFile has a virtual override, use that instead
		PlisgoFileInfo info;

		if (PlisgoVirtualGetFileInfo(pVirtualFile, &info) == 0)
			PrintPlisgoFileInfo(&info, pPacket->nDepth);

		++(pPacket->nDepth);
		PlisgoVirtualFileForEachChild(pVirtualFile, DumpVirtualTreeCB, pData);
		--(pPacket->nDepth);

		PlisgoVirtualFileClose(pVirtualFile);
	}
	else
	{
		//This BasicFile comes as is

		PrintBasicFile(sName, pFile, pPacket->nDepth);

		++(pPacket->nDepth);
		BasicFolder_ForEachChild(pFile, DumpCombinedTreeCB, pData);
		--(pPacket->nDepth);
	}

	pPacket->sPath[nPos] = L'\0';

	return TRUE;
}




/*
	Below is a test callback (and packet) that prints out the virtual file tree
*/


int _tmain(int argc, _TCHAR* argv[])
{
	PlisgoFolder* pPlisgoFolder = NULL;
	PlisgoFiles* pPlisgoFiles = NULL;	
	BasicFile* pRoot = NULL;
	int nError = 0;
	ULONG64	nNow;

	pRoot = BasicFolder_Create();

	assert(pRoot != NULL);

	GetSystemTimeAsFileTime((FILETIME*)&nNow);

	CreateTestTree(pRoot);

	nError = PlisgoFilesCreate(&pPlisgoFiles, GetBasicFSPlisgoToHostCBs(pRoot));

	assert(nError == 0 && pPlisgoFiles != NULL);

	pPlisgoFolder = GetBacisFSUI_PlisgoFolder(pPlisgoFiles, pRoot, L"\\");

	//Dump combined tree
	{
		DumpTreePacket packet = {0};

		packet.pPlisgoFiles = pPlisgoFiles;

		DumpCombinedTreeCB(L"", pRoot, &packet);
	}


	if (pPlisgoFolder != NULL)
		PlisgoFolderDestroy(pPlisgoFolder);
	
	PlisgoFilesDestroy(pPlisgoFiles);
	BasicFile_Release(pRoot);

	if (GetTotalBasicFileNum())
		wprintf(L"\n\n SOME 'BasicFile's HAVE BEEN LEEKED");

	return 0;
}

