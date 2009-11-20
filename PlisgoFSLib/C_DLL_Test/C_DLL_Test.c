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




BOOL TestPlisgoFindInShelledFolderCB(LPCWSTR sFolderPath, UINT nRequestIndex, WCHAR* sNameBuffer, UINT nBufferSize, BOOL* pIsFolder, void* pData)
{
/*
	In our fake virtaul file system, we have one file and one folder containg one file, we will to shell these, which is to say
	we wish to have a custom column entry, overlay icon, custom icon or a thumbnail
*/
	if (sNameBuffer == NULL)
		return TRUE; //A NULL sNameBuffer means it's just checking if this is a shelled folder at all.


	if (pIsFolder == NULL)
		return FALSE;

	if (sFolderPath == NULL || sFolderPath[0] == L'\0')
		return FALSE;

	if (sFolderPath[0] == L'\\' && sFolderPath[1] == L'\0')
	{
		if (nRequestIndex == 0)
		{
			wsprintf(sNameBuffer, L"test");

			*pIsFolder = FALSE;

			return TRUE;
		}
		else if (nRequestIndex == 1)
		{
			wsprintf(sNameBuffer, L"testFolder");

			*pIsFolder = TRUE;

			return TRUE;
		}
	}
	else if (wcscmp(sFolderPath, L"\\testFolder\\") == 0)
	{
		if (nRequestIndex == 0)
		{
			wsprintf(sNameBuffer, L"test2");

			*pIsFolder = FALSE;

			return TRUE;
		}
		else if (nRequestIndex == 1)
		{
			wsprintf(sNameBuffer, L"testFolder2");

			*pIsFolder = TRUE;

			return TRUE;
		}
	}
	else if (wcscmp(sFolderPath, L"\\testFolder\\testFolder2\\") == 0)
	{
		if (nRequestIndex == 0)
		{
			wsprintf(sNameBuffer, L"test3");

			*pIsFolder = FALSE;

			return TRUE;
		}
	}

	return FALSE;
}


BOOL TestPlisgoGetColumnEntryCB(LPCWSTR sFile, UINT nColumn, WCHAR* sBuffer, UINT nBufferSize, void* pData)
{
/*
	We have one custom column (called "Test Column") and here we get the entry for that column for the file test at the root.
*/

	if (nColumn != 0)
		return FALSE;

	if (wcscmp(sFile, L"\\test") == 0)
	{
		wcscpy_s(sBuffer, nBufferSize, L"test-comment");
		return TRUE;
	}
	
	if (wcscmp(sFile, L"\\testFolder") == 0)
	{
		wcscpy_s(sBuffer, nBufferSize, L"testFolder-comment");
		return TRUE;
	}

	if (wcscmp(sFile, L"\\testFolder\\test2") == 0)
	{
		wcscpy_s(sBuffer, nBufferSize, L"test2-comment");
		return TRUE;
	}

	if (wcscmp(sFile, L"\\testFolder\\testFolder2\\test3") == 0)
	{
		wcscpy_s(sBuffer, nBufferSize, L"test3-comment");
		return TRUE;
	}

	return FALSE;
}


BOOL TestPlisgoGetIconCB(LPCWSTR sFile, BOOL* pbUsesList, UINT* pnList, WCHAR* sPathBuffer, UINT nBufferSize, UINT* pnEntryIndex, void* pData)
{
	/*
		For both the overlay icon and the custom icon, for the file test at the root, we use the first image in the first image list
		Which in the real world is pointless, but hopefully you see how this works, you would have a different functions or
		at least different results for the custom icon and overlay.
	*/
	if (wcscmp(sFile, L"\\test") != 0)
		return FALSE;

	if (pnList == NULL || pnEntryIndex == NULL)
		return FALSE;

	*pnList = 0;
	*pnEntryIndex= 0;

	return TRUE;
}


BOOL TestPlisgoGetThumbnailCB(LPCWSTR sFile, LPCWSTR sThumbnailExt, WCHAR* sPathBuffer, UINT nBufferSize, void* pData)
{
	//We only do bmp thumbnails, and we only have a thumbnail for the file test at the root
	LPCWSTR sLowerBmp = L".bmp";

	if (wcscmp(sFile, L"\\test") != 0)
		return FALSE;

	for(;*sLowerBmp != L'\0' && *sThumbnailExt != L'\0';++sLowerBmp,++sThumbnailExt)
		if (tolower(*sThumbnailExt) != *sLowerBmp)
			return FALSE;

	if (*sLowerBmp != L'\0' || *sThumbnailExt != L'\0')
		return FALSE;	

	if (sPathBuffer == NULL)
		return FALSE;

	wcscpy_s(sPathBuffer, nBufferSize, L"C:\\Windows\\Blue Lace 16.bmp");

	return TRUE;
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


/*
	Below is a test callback (and packet) that prints out the virtual file tree
*/

struct TestPlisgoVirtualFileChildPacket
{
	PlisgoFiles*	pPlisgoFiles;
	LPCWSTR			sParent;
};
typedef struct TestPlisgoVirtualFileChildPacket TestPlisgoVirtualFileChildPacket;


int TestPlisgoVirtualFileChildCB(	LPCWSTR		sName,
									DWORD		nAttr,
									ULONGLONG	nSize,
									ULONGLONG	nCreation,
									ULONGLONG	nLastAccess,
									ULONGLONG	nLastWrite,
									void*		pUserData)
{
	TestPlisgoVirtualFileChildPacket* pPacket = (TestPlisgoVirtualFileChildPacket*)pUserData;

	WCHAR sPath[MAX_PATH];

	wsprintf(sPath, L"%s\\%s", pPacket->sParent, sName);

	if (nAttr & FILE_ATTRIBUTE_DIRECTORY)
	{
		TestPlisgoVirtualFileChildPacket packet;
		PlisgoVirtualFile* pFolder = NULL;

		wprintf(L"%s %i\n", sPath, nAttr);

		PlisgoVirtualFileOpen(pPacket->pPlisgoFiles, sPath, NULL, &pFolder, GENERIC_READ, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS);

		packet.pPlisgoFiles = pPacket->pPlisgoFiles;
		packet.sParent = sPath;

		PlisgoVirtualFileForEachChild(pFolder, TestPlisgoVirtualFileChildCB, &packet);
		PlisgoVirtualFileClose(pFolder);
	}
	else
	{
		wprintf(L"%s %i %I64u\n", sPath, nAttr, nSize);
	}

	return 0;
}



int _tmain(int argc, _TCHAR* argv[])
{
	int nError = 0;
	PlisgoFiles* pPlisgoFiles = NULL;

	PlisgoGUICBs cbs = {0};

	cbs.FindInShelledFolderCB = &TestPlisgoFindInShelledFolderCB;
	cbs.GetColumnEntryCB = &TestPlisgoGetColumnEntryCB;
	cbs.GetCustomIconCB = &TestPlisgoGetIconCB;
	cbs.GetOverlayIconCB = &TestPlisgoGetIconCB;
	cbs.GetThumbnailCB = &TestPlisgoGetThumbnailCB;


	nError = PlisgoFilesCreate(&pPlisgoFiles, L"test", &cbs);

	if (nError == 0 && pPlisgoFiles != NULL)
	{
		TestPlisgoVirtualFileChildPacket packet;
		int nMenuItem = 0;

		PlisgoFilesAddColumn(pPlisgoFiles, L"Test Column");
		PlisgoFilesSetColumnType(pPlisgoFiles, 0, 1);
		PlisgoFilesSetColumnAlignment(pPlisgoFiles, 0, 1);
		PlisgoFilesAddIconsList(pPlisgoFiles, L"C:\\Windows\\Blue Lace 16.bmp");

		PlisgoFilesAddCustomFolderIcon(pPlisgoFiles, 0, 0, 0, 0);
		PlisgoFilesAddCustomDefaultIcon(pPlisgoFiles, 0 , 0);
		PlisgoFilesAddCustomExtensionIcon(pPlisgoFiles, L".txt", 0, 0);

		PlisgoFilesAddMenuItem(pPlisgoFiles, &nMenuItem, L"test", &TestOnClickCB, -1, &TestIsEnabledCB, 0, 0, NULL);
		PlisgoFilesAddMenuSeparatorItem(pPlisgoFiles, -1);
		PlisgoFilesAddMenuItem(pPlisgoFiles, &nMenuItem, L"test2", &TestOnClickCB, -1, &TestIsEnabledCB, 0, 0, NULL);
		PlisgoFilesAddMenuItem(pPlisgoFiles, &nMenuItem, L"test3", &TestOnClickCB, nMenuItem, &TestIsEnabledCB, 0, 0, NULL); //Child menu item


		packet.pPlisgoFiles = pPlisgoFiles;
		packet.sParent = L"";

		PlisgoFilesForRootFiles(pPlisgoFiles, TestPlisgoVirtualFileChildCB, &packet);


		PlisgoFilesDestroy(pPlisgoFiles);
	}


	return 0;
}

