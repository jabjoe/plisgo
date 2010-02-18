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
#include <tchar.h>
#include <assert.h>
#include <windows.h>

#include "../PlisgoFSLib_DLL.h"
#include "Basic_FS.h"
#include "Basic_FS_UI.h"


/*
If this was a real filesystem, then when you clicked on the plisgo menu item, this function would be called.
*/
BOOL TestOnClickCB(LPCWSTR sFile, void* pData)
{
	//Our menu item has been click on

	return TRUE;
}

/*
If this was a real filesystem, this would be called when the plisgo menu is display. This call determins if the item is visable or not.
*/
BOOL TestIsEnabledCB(LPCWSTR sFile, void* pData)
{
	//We want our menu item enabled regardless of what file

	return TRUE;
}


/*
This function is used by plisgo to query if we wish to shell this file/folder or not.
*/
static BOOL TestIsShelledCB(LPCWSTR sPath, void* pData)
{
	BasicFile* pFile;

	assert(sPath != NULL && pData != NULL);
	
	pFile = BasicFolder_WalkPath((BasicFile*)pData, sPath);

	if (pFile == NULL) //No file to work on!
		return FALSE;

	BasicFile_Release(pFile);

	return TRUE;
}

/*
This function queries what the column entry for this file is, return true if there is one, false if not.
*/
static BOOL TestPlisgoGetColumnEntryCB(LPCWSTR sPath, UINT nColumn, WCHAR sBuffer[MAX_PATH], void* pData)
{
	BasicFile* pFile;

	assert(sPath != NULL && pData != NULL);
	
	pFile = BasicFolder_WalkPath((BasicFile*)pData, sPath);

	if (pFile == NULL)
		return FALSE;

	BasicFile_Release(pFile);

	wsprintf(sBuffer,L"column entry %i for %s", nColumn, sPath);

	return TRUE;

}

/*
This function queries what the current overlay for a file is, return true if there is one, false if not.
In this case we choice to use a icon from the plisgo icon list, and it's the first icon of the first list
*/
static BOOL TestPlisgoGetOverlayIconCB(LPCWSTR sPath, BOOL* pbUsesList, UINT* pnList, WCHAR sPathBuffer[MAX_PATH], UINT* pnEntryIndex, void* pData)
{
	BasicFile* pFile;

	assert(sPath != NULL && pData != NULL && pbUsesList != NULL && pnList != NULL);

	pFile = BasicFolder_WalkPath((BasicFile*)pData, sPath);

	if (pFile == NULL)
		return FALSE;

	BasicFile_Release(pFile);

	*pbUsesList = TRUE;
	*pnList = 0;
	*pnEntryIndex = 0;

	return TRUE;
}

/*
This function queries what the current custom icon for a file is, return true if there is one, false if not.
In this case we choice to use a existing icon.
*/
static BOOL TestPlisgoGetCustomIconCB(LPCWSTR sPath, BOOL* pbUsesList, UINT* pnList, WCHAR sPathBuffer[MAX_PATH], UINT* pnEntryIndex, void* pData)
{
	BasicFile* pFile;
	
	assert(sPath != NULL && pData != NULL && pbUsesList != NULL && pnList != NULL);

	pFile = BasicFolder_WalkPath((BasicFile*)pData, sPath);

	if (pFile == NULL)
		return FALSE;

	BasicFile_Release(pFile);

	*pbUsesList = FALSE;

	wcscpy_s(sPathBuffer, MAX_PATH, L"C:\\Windows\\System32\\shell32.dll");
	*pnEntryIndex = 0;

	return TRUE;
}


/*
This function queries what the current thumbnail for a file is, return true if there is one, false if not.
In this case it's a bmp file
*/
static BOOL TestPlisgoGetThumbnailCB(LPCWSTR sPath, WCHAR sPathBuffer[MAX_PATH], void* pData)
{
	BasicFile* pFile;
	
	assert(sPath != NULL && pData != NULL);

	pFile = BasicFolder_WalkPath((BasicFile*)pData, sPath);

	if (pFile == NULL)
		return FALSE;

	wcscpy_s(sPathBuffer, MAX_PATH, L"C:\\WINDOWS\\Soap Bubbles.bmp");

	BasicFile_Release(pFile);

	return TRUE;
}


/*
This function create and sets up the PlisgoFolder structure for the example UI
*/


PlisgoFolder*	GetBacisFSUI_PlisgoFolder(PlisgoFiles* pPlisgoFiles, BasicFile* pRoot, LPCWSTR sMount)
{
	static PlisgoGUICBs UICBs;
	PlisgoFolder* pResult = NULL;
	BasicFile* pMount = NULL;
	BasicFile* pStub = NULL;

	assert(pPlisgoFiles != NULL);
	assert(pRoot != NULL);
	assert(sMount != NULL);

	UICBs.IsShelledFolderCB	= &TestIsShelledCB;
	UICBs.GetColumnEntryCB	= &TestPlisgoGetColumnEntryCB;
	UICBs.GetCustomIconCB	= &TestPlisgoGetCustomIconCB;
	UICBs.GetOverlayIconCB	= &TestPlisgoGetOverlayIconCB;
	UICBs.GetThumbnailCB	= &TestPlisgoGetThumbnailCB;
	UICBs.pUserData			= pRoot; //All out functions require the root as the userdata

	wcscpy_s(UICBs.sFSName, MAX_PATH, L"BasicFS");

	pMount = BasicFolder_WalkPath(pRoot, sMount);

	if (pMount == NULL)
		return NULL;

	if (!BasicFile_IsFolder(pMount))
	{
		BasicFile_Release(pMount);

		return NULL;
	}

	pStub = BasicFile_Create();

	assert(pStub != NULL);

	//Add stubs for Plisgo to mount the required virtual files at
	BasicFolder_AddChild(pMount, L".plisgofs", pStub);
	BasicFolder_AddChild(pMount, L"Desktop.ini", pStub);

	BasicFile_Release(pStub);
	BasicFile_Release(pMount);

	PlisgoFolderCreate(pPlisgoFiles, &pResult, sMount, &UICBs); 		

	if (pResult != NULL)
	{
		int nMenuItem;

		//Add a icon list (in this case a file that only has one icon (square not a strip))
		PlisgoFolderAddIconsList(pResult, L"C:\\Windows\\Blue Lace 16.bmp");

		//Add a custom column
		PlisgoFolderAddColumn(pResult, L"Test Column");
		PlisgoFolderSetColumnType(pResult, 0, 1); //0 is text, 1 is int, 2 is float
		PlisgoFolderSetColumnAlignment(pResult, 0, 1);//-1 is left, 0 is center, 1 is right

		//Change some standard icons, all to use our icon from the our icon list
		PlisgoFolderAddCustomFolderIcon(pResult, 0, 0, 0, 0);
		PlisgoFolderAddCustomDefaultIcon(pResult, 0 , 0);
		PlisgoFolderAddCustomExtensionIcon(pResult, L".txt", 0, 0);

		//Add some menu items
		PlisgoFolderAddMenuItem(pResult, &nMenuItem, L"testMenu", &TestOnClickCB, -1, &TestIsEnabledCB, 0, 0, NULL);
		PlisgoFolderAddMenuSeparatorItem(pResult, -1);
		PlisgoFolderAddMenuItem(pResult, &nMenuItem, L"testMenu2", &TestOnClickCB, -1, &TestIsEnabledCB, 0, 0, NULL);
		//Child menu item
		PlisgoFolderAddMenuItem(pResult, &nMenuItem, L"testMenu3", &TestOnClickCB, /*->*/nMenuItem/*<-*/, &TestIsEnabledCB, 0, 0, NULL);
	}

	return pResult;
}