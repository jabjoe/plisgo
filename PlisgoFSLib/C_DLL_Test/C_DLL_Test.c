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




#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target other versions of Windows.
#endif

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include "../PlisgoFSLib_DLL.h"
#include "Basic_FS.h"







BOOL TestIsShelledCB(LPCWSTR sPath, void* pData)
{
	BasicFile* pFile = BasicFolder_WalkPath((BasicFile*)pData, sPath);

	if (pFile == NULL)
		return FALSE;

	BasicFile_Release(pFile);

	return TRUE;
}


BOOL TestPlisgoGetColumnEntryCB(LPCWSTR sFile, UINT nColumn, WCHAR sBuffer[MAX_PATH], void* pData)
{
	BasicFile* pFile = BasicFolder_WalkPath((BasicFile*)pData, sFile);

	if (pFile == NULL)
		return FALSE;

	BasicFile_Release(pFile);

	return FALSE;

}


BOOL TestPlisgoGetOverlayIconCB(LPCWSTR sFile, BOOL* pbUsesList, UINT* pnList, WCHAR sPathBuffer[MAX_PATH], UINT* pnEntryIndex, void* pData)
{
	BasicFile* pFile = BasicFolder_WalkPath((BasicFile*)pData, sFile);

	if (pFile == NULL)
		return FALSE;

	BasicFile_Release(pFile);

	return FALSE;
}


BOOL TestPlisgoGetCustomIconCB(LPCWSTR sFile, BOOL* pbUsesList, UINT* pnList, WCHAR sPathBuffer[MAX_PATH], UINT* pnEntryIndex, void* pData)
{
	BasicFile* pFile = BasicFolder_WalkPath((BasicFile*)pData, sFile);

	if (pFile == NULL)
		return FALSE;

	BasicFile_Release(pFile);

	return FALSE;
}



BOOL TestPlisgoGetThumbnailCB(LPCWSTR sFile, WCHAR sThumbnailExt[4], WCHAR sPathBuffer[MAX_PATH], void* pData)
{
	BasicFile* pFile = BasicFolder_WalkPath((BasicFile*)pData, sFile);

	if (pFile == NULL)
		return FALSE;

	wcscpy_s(sThumbnailExt, 4, L"jpg");

	BasicFile_Release(pFile);

	return FALSE;
}


BOOL TestOnClickCB(LPCWSTR sFile, void* pData)
{
	//Our menu item has been click on

	return TRUE;
}


BOOL TestIsEnabledCB(LPCWSTR sFile, void* pData)
{
	//We want our menu item enabled regardless of what file

	return TRUE;
}



struct BasicFS_HostFolder
{
	BasicFile*	pFile;
	UINT		nChildIndex;
};
typedef struct BasicFS_HostFolder BasicFS_HostFolder;



BOOL BasicFS_OpenHostFolderCB(LPCWSTR sPath, void** pHostFolderData, void* pData)
{
	BasicFile* pFile = BasicFolder_WalkPath((BasicFile*)pData, sPath);
	BasicFS_HostFolder* pBasicFS_Folder ;

	if (pFile == NULL)
		return FALSE;

	pBasicFS_Folder = (BasicFS_HostFolder*)malloc(sizeof(BasicFS_HostFolder));

	*pHostFolderData = pBasicFS_Folder;

	pBasicFS_Folder->pFile = pFile;
	pBasicFS_Folder->nChildIndex = 0;

	return TRUE;
}


BOOL BasicFS_NextHostFolderChildCB(void* pHostFolderData, WCHAR sChildName[MAX_PATH], BOOL* pbIsFolder, void* pData)
{
	BasicFS_HostFolder* pBasicFS_Folder = (BasicFS_HostFolder*)pHostFolderData;
	BasicFile* pFile;

	if (!BasicFolder_GetChildName(pBasicFS_Folder->pFile,  pBasicFS_Folder->nChildIndex, sChildName))
		return FALSE;

	pFile = BasicFolder_GetChildByIndex(pBasicFS_Folder->pFile, pBasicFS_Folder->nChildIndex);

	if (pFile == NULL)
		return FALSE;

	(pBasicFS_Folder->nChildIndex)++;

	*pbIsFolder = BasicFile_IsFolder(pFile);

	BasicFile_Release(pFile);

	return TRUE;
}


BOOL BasicFS_QueryHostFolderChildCB(void* pHostFolderData, LPCWSTR sChildName, BOOL* pbIsFolder, void* pData)
{
	BasicFS_HostFolder* pBasicFS_Folder = (BasicFS_HostFolder*)pHostFolderData;

	BasicFile* pChild = BasicFolder_GetChild(pBasicFS_Folder->pFile, sChildName);

	if (pChild == NULL)
		return FALSE;

	*pbIsFolder = BasicFile_IsFolder(pChild);

	BasicFile_Release(pChild);

	return TRUE;
}


BOOL BasicFS_CloseHostFolderCB(void* pHostFolderData, void* pData)
{
	BasicFS_HostFolder* pBasicFS_Folder = (BasicFS_HostFolder*)pHostFolderData;

	BasicFile_Release(pBasicFS_Folder->pFile);

	free(pBasicFS_Folder);

	return TRUE;
}










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

	PlisgoGUICBs	shellCBs = {0};
	PlisgoToHostCBs	hostCBS = {0};

	shellCBs.IsShelledFolderCB	= &TestIsShelledCB;
	shellCBs.GetColumnEntryCB	= &TestPlisgoGetColumnEntryCB;
	shellCBs.GetCustomIconCB	= &TestPlisgoGetCustomIconCB;
	shellCBs.GetOverlayIconCB	= &TestPlisgoGetOverlayIconCB;
	shellCBs.GetThumbnailCB		= &TestPlisgoGetThumbnailCB;

	hostCBS.OpenHostFolderCB		= BasicFS_OpenHostFolderCB;
	hostCBS.NextHostFolderChildCB	= BasicFS_NextHostFolderChildCB;
	hostCBS.QueryHostFolderChildCB	= BasicFS_QueryHostFolderChildCB;
	hostCBS.CloseHostFolderCB		= BasicFS_CloseHostFolderCB;

	pRoot = BasicFile_Create();

	shellCBs.pUserData = pRoot;
	hostCBS.pUserData = pRoot;

	BasicFile_SetAsFolder(pRoot);

	CreateTestTree(pRoot, 0);



	nError = PlisgoFilesCreate(&pPlisgoFiles, &hostCBS);

	if (nError == 0 && pPlisgoFiles != NULL)
	{
		PlisgoVirtualFile* pVirtualFile = NULL;
		PlisgoFolder* pPlisgoFolder = NULL;
		int nMenuItem = 0;

		PlisgoFolderCreate(pPlisgoFiles, &pPlisgoFolder, L"", L"test", &shellCBs); 		

		if (pPlisgoFolder != NULL)
		{
			PlisgoFolderAddColumn(pPlisgoFolder, L"Test Column");
			PlisgoFolderSetColumnType(pPlisgoFolder, 0, 1);
			PlisgoFolderSetColumnAlignment(pPlisgoFolder, 0, 1);
			PlisgoFolderAddIconsList(pPlisgoFolder, L"C:\\Windows\\Blue Lace 16.bmp");

			PlisgoFolderAddCustomFolderIcon(pPlisgoFolder, 0, 0, 0, 0);
			PlisgoFolderAddCustomDefaultIcon(pPlisgoFolder, 0 , 0);
			PlisgoFolderAddCustomExtensionIcon(pPlisgoFolder, L".txt", 0, 0);

			PlisgoFolderAddMenuItem(pPlisgoFolder, &nMenuItem, L"testMenu", &TestOnClickCB, -1, &TestIsEnabledCB, 0, 0, NULL);
			PlisgoFolderAddMenuSeparatorItem(pPlisgoFolder, -1);
			PlisgoFolderAddMenuItem(pPlisgoFolder, &nMenuItem, L"testMenu2", &TestOnClickCB, -1, &TestIsEnabledCB, 0, 0, NULL);
			PlisgoFolderAddMenuItem(pPlisgoFolder, &nMenuItem, L"testMenu3", &TestOnClickCB, nMenuItem, &TestIsEnabledCB, 0, 0, NULL); //Child menu item


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

