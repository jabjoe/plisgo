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
#include <assert.h>
#include <windows.h>

#include "../PlisgoFSLib_DLL.h"
#include "Basic_FS.h"
#include "Basic_FS_UI.h"





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





static BOOL TestIsShelledCB(LPCWSTR sPath, void* pData)
{
	BasicFile* pFile = BasicFolder_WalkPath((BasicFile*)pData, sPath);

	if (pFile == NULL) //No file to work on!
		return FALSE;

	BasicFile_Release(pFile);

	return TRUE;
}


static BOOL TestPlisgoGetColumnEntryCB(LPCWSTR sFile, UINT nColumn, WCHAR sBuffer[MAX_PATH], void* pData)
{
	BasicFile* pFile = BasicFolder_WalkPath((BasicFile*)pData, sFile);

	if (pFile == NULL)
		return FALSE;

	BasicFile_Release(pFile);

	wsprintf(sBuffer,L"column entry %i for %s", nColumn, sFile);

	return TRUE;

}


static BOOL TestPlisgoGetOverlayIconCB(LPCWSTR sFile, BOOL* pbUsesList, UINT* pnList, WCHAR sPathBuffer[MAX_PATH], UINT* pnEntryIndex, void* pData)
{
	BasicFile* pFile = BasicFolder_WalkPath((BasicFile*)pData, sFile);

	if (pFile == NULL)
		return FALSE;

	BasicFile_Release(pFile);

	*pbUsesList = TRUE;
	*pnList = 1;
	*pnEntryIndex = 0;

	return TRUE;
}


static BOOL TestPlisgoGetCustomIconCB(LPCWSTR sFile, BOOL* pbUsesList, UINT* pnList, WCHAR sPathBuffer[MAX_PATH], UINT* pnEntryIndex, void* pData)
{
	BasicFile* pFile = BasicFolder_WalkPath((BasicFile*)pData, sFile);

	if (pFile == NULL)
		return FALSE;

	BasicFile_Release(pFile);

	*pbUsesList = TRUE;
	*pnList = 0;
	*pnEntryIndex = 0;

	return TRUE;
}



static BOOL TestPlisgoGetThumbnailCB(LPCWSTR sFile, WCHAR sThumbnailExt[4], WCHAR sPathBuffer[MAX_PATH], void* pData)
{
	BasicFile* pFile = BasicFolder_WalkPath((BasicFile*)pData, sFile);

	if (pFile == NULL)
		return FALSE;

	wcscpy_s(sThumbnailExt, 4, L"bmp");
	wcscpy_s(sPathBuffer, MAX_PATH, L"C:\\WINDOWS\\Soap Bubbles.bmp");

	BasicFile_Release(pFile);

	return TRUE;
}


PlisgoFolder*	GetBacisFSUI_PlisgoFolder(PlisgoFiles* pPlisgoFiles, BasicFile* pRoot)
{
	static PlisgoGUICBs UICBs;
	PlisgoFolder* pResult = NULL;

	assert(pPlisgoFiles != NULL);
	assert(pRoot != NULL);

	UICBs.IsShelledFolderCB= &TestIsShelledCB;
	UICBs.GetColumnEntryCB	= &TestPlisgoGetColumnEntryCB;
	UICBs.GetCustomIconCB	= &TestPlisgoGetCustomIconCB;
	UICBs.GetOverlayIconCB	= &TestPlisgoGetOverlayIconCB;
	UICBs.GetThumbnailCB	= &TestPlisgoGetThumbnailCB;
	UICBs.pUserData			= pRoot;

	wcscpy_s(UICBs.sFSName, MAX_PATH, L"BasicFS");


	PlisgoFolderCreate(pPlisgoFiles, &pResult, L"", L"test", &UICBs); 		

	if (pResult != NULL)
	{
		int nMenuItem;

		PlisgoFolderAddColumn(pResult, L"Test Column");
		PlisgoFolderSetColumnType(pResult, 0, 1);
		PlisgoFolderSetColumnAlignment(pResult, 0, 1);
		PlisgoFolderAddIconsList(pResult, L"C:\\Windows\\Blue Lace 16.bmp");

		PlisgoFolderAddCustomFolderIcon(pResult, 0, 0, 0, 0);
		PlisgoFolderAddCustomDefaultIcon(pResult, 0 , 0);
		PlisgoFolderAddCustomExtensionIcon(pResult, L".txt", 0, 0);

		PlisgoFolderAddMenuItem(pResult, &nMenuItem, L"testMenu", &TestOnClickCB, -1, &TestIsEnabledCB, 0, 0, NULL);
		PlisgoFolderAddMenuSeparatorItem(pResult, -1);
		PlisgoFolderAddMenuItem(pResult, &nMenuItem, L"testMenu2", &TestOnClickCB, -1, &TestIsEnabledCB, 0, 0, NULL);
		PlisgoFolderAddMenuItem(pResult, &nMenuItem, L"testMenu3", &TestOnClickCB, nMenuItem, &TestIsEnabledCB, 0, 0, NULL); //Child menu item
	}

	return pResult;
}