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



#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#include "../PlisgoFSLib_DLL.h"
#include "Basic_FS.h"
#include "Basic_FS_UI.h"
#include "Basic_FS_Host.h"





struct DumpTreePacket
{
	int				nDepth;
	PlisgoFiles*	pPlisgoFiles;
	WCHAR			sPath[MAX_PATH];
};
typedef struct DumpTreePacket DumpTreePacket;


static BOOL DumpTreeCB(PlisgoFileInfo* pPlisgoFileInfo, void* pData)
{
	DumpTreePacket* pPacket = (DumpTreePacket*)pData;

	int nDepth = pPacket->nDepth;
	BOOL bFolder = ((pPlisgoFileInfo->nAttr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)?TRUE:FALSE;

	while(nDepth--)
		wprintf(L"    ");

	wprintf(L"%s %s: %I64u\n", pPlisgoFileInfo->sName, (bFolder)?L"(folder)":L"", pPlisgoFileInfo->nSize);

	if (bFolder)
	{
		size_t nPos = wcslen(pPacket->sPath);

		PlisgoVirtualFile* pVirtualFile = NULL;

		++pPacket->nDepth;

		wcscat_s(pPacket->sPath, MAX_PATH, L"\\");
		wcscat_s(pPacket->sPath, MAX_PATH, pPlisgoFileInfo->sName);

		PlisgoVirtualFileOpen(pPacket->pPlisgoFiles, pPacket->sPath, &pVirtualFile, GENERIC_READ, 0, OPEN_EXISTING, 0);

		if (pVirtualFile != NULL)
		{
			PlisgoVirtualFileForEachChild(pVirtualFile, DumpTreeCB, pPacket);

			PlisgoVirtualFileClose(pVirtualFile);
		}

		pPacket->sPath[nPos] = L'\0';


		--pPacket->nDepth;
	}

	return TRUE;
}


BOOL DumpBasicFSTreeCB(LPCWSTR sName, BasicFile* pFile, void* pData)
{
	int nDepth = *(int*)pData;
	BOOL bFolder;
	BasicStats stats;

	while(nDepth--)
		wprintf(L"    ");

	BasicFile_GetStats(pFile, &stats);

	bFolder = ((stats.nAttr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);

	wprintf(L"%s %s: %I64u\n", sName, (bFolder)?L"(folder)":L"", stats.nSize);

	if (bFolder)
	{
		++(*(int*)pData);

		BasicFolder_ForEachChild(pFile, DumpBasicFSTreeCB, pData);

		--(*(int*)pData);
	}

	return TRUE;
}



static void CreateTestTree(BasicFile* pParent, int nDepth)
{
	BOOL bHasSubFolder = FALSE;
	WCHAR sName[MAX_PATH];
	int nChildNum;
	
	nChildNum = 1 + (rand() % 20);


	while(nChildNum--)
	{
		BasicFile* pTemp;

		pTemp = BasicFile_Create();

		wsprintf(sName, L"file_%i", rand()%100);

		BasicFolder_AddChild(pParent, sName, pTemp);

		if (nDepth < 4 && ((rand()%4 == 0) || (!bHasSubFolder && nChildNum == 0)))
		{
			BasicFile_SetAsFolder(pTemp);

			CreateTestTree(pTemp, nDepth+1);

			bHasSubFolder = TRUE;
		}

		BasicFile_Release(pTemp);
	}
}


/*
	Below is a test callback (and packet) that prints out the virtual file tree
*/


int _tmain(int argc, _TCHAR* argv[])
{
	int nError = 0;
	BasicFile* pRoot;
	PlisgoFiles* pPlisgoFiles = NULL;

	PlisgoToHostCBs	hostCBS = {0};


	GetBasicFSPlisgoToHostCBs(&hostCBS);

	pRoot = BasicFile_Create();

	hostCBS.pUserData = pRoot;

	BasicFile_SetAsFolder(pRoot);

	CreateTestTree(pRoot, 0);


	nError = PlisgoFilesCreate(&pPlisgoFiles, &hostCBS);

	if (nError == 0 && pPlisgoFiles != NULL)
	{
		PlisgoVirtualFile* pVirtualFile = NULL;
		PlisgoFolder* pPlisgoFolder = NULL;
		int nMenuItem = 0;

		pPlisgoFolder = GetBacisFSUI_PlisgoFolder(pPlisgoFiles, pRoot);

		if (pPlisgoFolder != NULL)
		{
			//Right it's all populated, dump the tree!

			//Open the root
			PlisgoVirtualFileOpen(pPlisgoFiles, L"", &pVirtualFile, GENERIC_READ, 0, OPEN_EXISTING, 0);

			if (pVirtualFile != NULL)
			{
				DumpTreePacket packet;

				packet.nDepth = 0;
				packet.pPlisgoFiles = pPlisgoFiles;
				packet.sPath[0] = L'\0';

				wprintf(L"\n\n======Extra \"Virtual\" filesystem =========\n\n");

				PlisgoVirtualFileForEachChild(pVirtualFile, DumpTreeCB, &packet);

				PlisgoVirtualFileClose(pVirtualFile);
			}

			{
				int nDepth = 0;

				wprintf(L"\n\n======\"Real\" filesystem =========\n\n");

				BasicFolder_ForEachChild(pRoot, DumpBasicFSTreeCB, &nDepth);
			}

			PlisgoFolderDestroy(pPlisgoFolder);
		}


		PlisgoFilesDestroy(pPlisgoFiles);
	}

	BasicFile_Release(pRoot);

	if (GetTotalBasicFileNum())
		wprintf(L"\n\n SOME 'BasicFile's HAVE BEEN LEEKED");

	return 0;
}

