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
#include "Basic_FS_Host.h"



struct BasicFS_HostFolder
{
	BasicFile*	pFile;
	UINT		nChildIndex;
};
typedef struct BasicFS_HostFolder BasicFS_HostFolder;



static BOOL BasicFS_OpenHostFolderCB(LPCWSTR sPath, void** pHostFolderData, void* pData)
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


static BOOL BasicFS_NextHostFolderChildCB(void* pHostFolderData, WCHAR sChildName[MAX_PATH], BOOL* pbIsFolder, void* pData)
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


static BOOL BasicFS_QueryHostFolderChildCB(void* pHostFolderData, LPCWSTR sChildName, BOOL* pbIsFolder, void* pData)
{
	BasicFS_HostFolder* pBasicFS_Folder = (BasicFS_HostFolder*)pHostFolderData;

	BasicFile* pChild = BasicFolder_GetChild(pBasicFS_Folder->pFile, sChildName);

	if (pChild == NULL)
		return FALSE;

	*pbIsFolder = BasicFile_IsFolder(pChild);

	BasicFile_Release(pChild);

	return TRUE;
}


static BOOL BasicFS_CloseHostFolderCB(void* pHostFolderData, void* pData)
{
	BasicFS_HostFolder* pBasicFS_Folder = (BasicFS_HostFolder*)pHostFolderData;

	BasicFile_Release(pBasicFS_Folder->pFile);

	free(pBasicFS_Folder);

	return TRUE;
}



void GetBasicFSPlisgoToHostCBs(PlisgoToHostCBs* pPlisgoToHostCBs)
{
	assert(pPlisgoToHostCBs != NULL);

	pPlisgoToHostCBs->OpenHostFolderCB			= BasicFS_OpenHostFolderCB;
	pPlisgoToHostCBs->NextHostFolderChildCB		= BasicFS_NextHostFolderChildCB;
	pPlisgoToHostCBs->QueryHostFolderChildCB	= BasicFS_QueryHostFolderChildCB;
	pPlisgoToHostCBs->CloseHostFolderCB			= BasicFS_CloseHostFolderCB;
}