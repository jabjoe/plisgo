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
#include "Basic_FS_Host.h"


/*
This is the structure we use to keep track of folders openned by Plisgo
*/
struct BasicFS_HostFolder
{
	BasicFile*	pFolder;
	UINT		nChildIndex;
};
typedef struct BasicFS_HostFolder BasicFS_HostFolder;


/*
This function is called when Plisgo opens the folder
*/
static BOOL BasicFS_OpenHostFolderCB(LPCWSTR sPath, void** ppHostFolderData, void* pData)
{
	BasicFile* pFile;
	BasicFS_HostFolder* pBasicFS_Folder ;

	assert(pData != NULL && sPath != NULL && ppHostFolderData != NULL);


	pFile = BasicFolder_WalkPath((BasicFile*)pData, sPath);

	if (pFile == NULL)
		return FALSE;

	pBasicFS_Folder = (BasicFS_HostFolder*)malloc(sizeof(BasicFS_HostFolder));

	*ppHostFolderData = pBasicFS_Folder;

	pBasicFS_Folder->pFolder = pFile;
	pBasicFS_Folder->nChildIndex = 0;

	return TRUE;
}

/*
This function is called while Plisgo is querying the children of the folder
*/
static BOOL BasicFS_NextHostFolderChildCB(void* pHostFolderData, WCHAR sChildName[MAX_PATH], BOOL* pbIsFolder, void* pData)
{
	BasicFS_HostFolder* pBasicFS_Folder = (BasicFS_HostFolder*)pHostFolderData;
	BasicFile* pFile;

	assert(pData != NULL && pbIsFolder != NULL && pHostFolderData != NULL);

	if (!BasicFolder_GetChildName(pBasicFS_Folder->pFolder,  pBasicFS_Folder->nChildIndex, sChildName))
		return FALSE;

	pFile = BasicFolder_GetChildByIndex(pBasicFS_Folder->pFolder, pBasicFS_Folder->nChildIndex);

	if (pFile == NULL)
		return FALSE;

	(pBasicFS_Folder->nChildIndex)++;

	*pbIsFolder = BasicFile_IsFolder(pFile);

	BasicFile_Release(pFile);

	return TRUE;
}

/*
This function is called while Plisgo is querying a specific child folder
*/
static BOOL BasicFS_QueryHostFolderChildCB(void* pHostFolderData, LPCWSTR sChildName, BOOL* pbIsFolder, void* pData)
{
	BasicFS_HostFolder* pBasicFS_Folder = (BasicFS_HostFolder*)pHostFolderData;
	BasicFile* pChild;

	assert(pData != NULL && pbIsFolder != NULL && pHostFolderData != NULL && sChildName != NULL);

	pChild = BasicFolder_GetChild(pBasicFS_Folder->pFolder, sChildName);

	if (pChild == NULL)
		return FALSE;

	*pbIsFolder = BasicFile_IsFolder(pChild);

	BasicFile_Release(pChild);

	return TRUE;
}

/*
This function is called while Plisgo has finished with the folder
*/
static BOOL BasicFS_CloseHostFolderCB(void* pHostFolderData, void* pData)
{
	BasicFS_HostFolder* pBasicFS_Folder = (BasicFS_HostFolder*)pHostFolderData;

	assert(pData != NULL && pHostFolderData != NULL);

	BasicFile_Release(pBasicFS_Folder->pFolder);

	free(pBasicFS_Folder);

	return TRUE;
}



PlisgoToHostCBs* GetBasicFSPlisgoToHostCBs(BasicFile* pRoot)
{
	static PlisgoToHostCBs hostCBS = {0};

	assert(pRoot != NULL);

	hostCBS.OpenHostFolderCB		= BasicFS_OpenHostFolderCB;
	hostCBS.NextHostFolderChildCB	= BasicFS_NextHostFolderChildCB;
	hostCBS.QueryHostFolderChildCB	= BasicFS_QueryHostFolderChildCB;
	hostCBS.CloseHostFolderCB		= BasicFS_CloseHostFolderCB;
	hostCBS.pUserData				= pRoot; //All out functions require the root as the userdata

	return &hostCBS;
}