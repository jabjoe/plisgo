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

#include "Basic_FS.h"


static ULONG nTotalFiles = 0;


struct BasicFile
{
	BasicStats	stats;
	void*		pData;
	ULONG		nRef;
};


struct FolderEntry
{
	WCHAR		sName[MAX_PATH];
	BasicFile*	pFile;
};
typedef struct FolderEntry FolderEntry;



UINT		GetTotalBasicFileNum()
{
	return nTotalFiles;
}


BasicFile*	BasicFile_Create()
{
	BasicFile* pResult = malloc(sizeof(BasicFile));

	if (pResult == NULL)
		return NULL;

	pResult->nRef = 1;
	pResult->pData = 0;
	pResult->stats.nSize = 0;
	pResult->stats.nAttr = FILE_ATTRIBUTE_NORMAL;

	GetSystemTimeAsFileTime((FILETIME*)&pResult->stats.nCreation);

	pResult->stats.nLastAccess = pResult->stats.nLastWrite = pResult->stats.nCreation;

	++nTotalFiles;

	return pResult;
}


void			BasicFile_GetStats(BasicFile* pFile, BasicStats* pStats)
{
	memcpy_s(pStats, sizeof(BasicStats), &pFile->stats, sizeof(BasicStats));

	if (BasicFile_IsFolder(pFile))
		pStats->nSize = 0;
}



static BOOL ReleaseFolderChildrenCB(LPCWSTR sName, BasicFile* pFile, void* pData)
{
	BasicFile_Release(pFile);

	return TRUE;
}


void			BasicFile_Release(BasicFile* pFile)
{
	--pFile->nRef;

	if (pFile->nRef == 0)
	{
		if (pFile->stats.nAttr == FILE_ATTRIBUTE_DIRECTORY)
			BasicFolder_ForEachChild(pFile, ReleaseFolderChildrenCB, NULL);

		if (pFile->pData != NULL)
			free(pFile->pData);

		free(pFile);

		--nTotalFiles;
	}
}


static void			BasicFile_WriteData_Internal(BasicFile* pFile, UINT nPos, void* pData, UINT nDataSize)
{
	if (nPos+nDataSize > pFile->stats.nSize)
	{
		pFile->pData = realloc(pFile->pData, nPos+nDataSize);

		memcpy_s(&(((BYTE*)pFile->pData)[nPos]), nDataSize, pData, nDataSize);

		pFile->stats.nSize = nPos+nDataSize;
	}
	else memcpy_s(&(((BYTE*)pFile->pData)[nPos]), nDataSize, pData, nDataSize);

	GetSystemTimeAsFileTime((FILETIME*)&pFile->stats.nLastWrite);
}


void			BasicFile_WriteData(BasicFile* pFile, UINT nPos, void* pData, UINT nDataSize)
{
	if (pFile->stats.nAttr == FILE_ATTRIBUTE_DIRECTORY)
		return;

	BasicFile_WriteData_Internal(pFile, nPos, pData, nDataSize);
}


static UINT			BasicFile_ReadData_Internal(BasicFile* pFile, UINT nPos, void* pData, UINT nDataSize)
{
	UINT	nReadEnd;
	BYTE*	pDst;
	BYTE*	pSrc;
	UINT	n;

	if (nPos >= pFile->stats.nSize)
		return 0;

	nReadEnd = nPos + nDataSize;

	if (nReadEnd > pFile->stats.nSize)
		nDataSize -= (UINT)(nReadEnd-pFile->stats.nSize);

	pDst = (BYTE*)pData;
	pSrc = &(((BYTE*)pFile->pData)[nPos]);

	n = nDataSize;

	while(n--)
		*pDst++ = *pSrc++;
	
	return nDataSize;
}


UINT			BasicFile_ReadData(BasicFile* pFile, UINT nPos, void* pData, UINT nDataSize)
{
	if (pFile->stats.nAttr == FILE_ATTRIBUTE_DIRECTORY)
		return 0;

	return BasicFile_ReadData_Internal(pFile, nPos, pData, nDataSize);
}



BOOL			BasicFile_SetAsFolder(BasicFile* pFile)
{
	if (pFile->stats.nSize != 0)
		return FALSE;

	pFile->stats.nAttr = FILE_ATTRIBUTE_DIRECTORY;

	return TRUE;
}


BOOL			BasicFile_IsFolder(BasicFile* pFile)
{
	return (pFile->stats.nAttr&FILE_ATTRIBUTE_DIRECTORY);
}


static UINT	BasicFolder_GetChildNum_Internal(BasicFile* pFolder)
{
	if (!BasicFile_IsFolder(pFolder))
		return 0;

	return (UINT)pFolder->stats.nSize / sizeof(FolderEntry);
}


static FolderEntry*		BasicFolder_GetChildEntry(BasicFile* pFolder, LPCWSTR sName)
{
	ULONG n;

	if (!BasicFile_IsFolder(pFolder))
		return NULL;

	n = BasicFolder_GetChildNum_Internal(pFolder);

	while(n--)
	{
		FolderEntry* pFolderEntry = &((FolderEntry*)pFolder->pData)[n];

		LPCWSTR sChildName = pFolderEntry->sName;
		LPCWSTR sPos = sName;

		while(sChildName[0] != L'\0' && sPos[0] != L'\0' && sPos[0] != L'\\')
		{
			if (tolower(sChildName[0]) != tolower(sPos[0]))
				break;

			++sChildName;
			++sPos;
		}

		if (sChildName[0] == L'\0' &&  (sPos[0] == L'\0' || sPos[0] == L'\\'))
			return pFolderEntry;
	}

	return NULL;
}


void			BasicFolder_AddChild(BasicFile* pFolder, LPCWSTR sName, BasicFile* pFile)
{
	FolderEntry* pEntry;

	if (!BasicFile_IsFolder(pFolder))
		return;

	pEntry = BasicFolder_GetChildEntry(pFolder, sName);

	if (pEntry != NULL && pEntry->pFile == NULL)
	{
		pEntry->pFile = pFile;
		return;
	}
	
	{
		FolderEntry entry;

		wcscpy_s(entry.sName, MAX_PATH, sName);
		entry.pFile = pFile;

		BasicFile_WriteData_Internal(pFolder, (UINT)pFolder->stats.nSize, &entry, sizeof(entry));

		++(pFile->nRef);
	}
}


void			BasicFolder_RemoveChild(BasicFile* pFolder, LPCWSTR sName)
{
	FolderEntry* pFolderEntry = BasicFolder_GetChildEntry(pFolder, sName);

	if (pFolderEntry->pFile != NULL)
	{
		BasicFile_Release(pFolderEntry->pFile);
		pFolderEntry->pFile = NULL;
	}
}


BasicFile*		BasicFolder_GetChild(BasicFile* pFolder, LPCWSTR sName)
{
	FolderEntry* pFolderEntry = BasicFolder_GetChildEntry(pFolder, sName);

	if (pFolderEntry == NULL)
		return NULL;

	pFolderEntry->pFile->nRef++; //This means it won't disable if removed from parent, but also means it must be released!

	return pFolderEntry->pFile;
}


BasicFile*		BasicFolder_WalkPath(BasicFile* pCurrent, LPCWSTR sPath)
{
	LPCWSTR sNext;

	if (!BasicFile_IsFolder(pCurrent))
		return NULL;

	if (sPath[0] == L'\\')
		++sPath;

	if (sPath[0] == L'\0')
	{
		pCurrent->nRef++;

		return pCurrent;
	}

	sNext = wcschr(sPath, L'\\');

	while(sNext != NULL)
	{
		FolderEntry* pFolderEntry = BasicFolder_GetChildEntry(pCurrent, sPath);

		if (pFolderEntry == NULL)
			return NULL;

		if (!BasicFile_IsFolder(pFolderEntry->pFile))
			return NULL;

		pCurrent = pFolderEntry->pFile;

		sPath = sNext+1;
		sNext = wcschr(sPath, L'\\');
	}

	return BasicFolder_GetChild(pCurrent, sPath);
}


void			BasicFolder_ForEachChild(BasicFile* pFolder, BasicFolder_ForEachChildCB cb, void* pData)
{
	ULONG n;

	if (!BasicFile_IsFolder(pFolder))
		return;

	n = BasicFolder_GetChildNum_Internal(pFolder);

	while(n--)
	{
		FolderEntry* pFolderEntry = &((FolderEntry*)pFolder->pData)[n];

		if (pFolderEntry->pFile != NULL && !cb(pFolderEntry->sName, pFolderEntry->pFile, pData))
			return;
	}
}


BasicFile*	BasicFolder_GetChildByIndex(BasicFile* pFolder, int nIndex)
{
	ULONG n;

	if (!BasicFile_IsFolder(pFolder))
		return NULL;

	n = BasicFolder_GetChildNum_Internal(pFolder);

	while(n--)
	{
		FolderEntry* pFolderEntry = &((FolderEntry*)pFolder->pData)[n];

		if (pFolderEntry->pFile != NULL)
		{
			if (nIndex == 0)
			{
				pFolderEntry->pFile->nRef++;

				return pFolderEntry->pFile;
			}
			else --nIndex;
		}
	}

	return NULL;
}


BOOL			BasicFolder_GetChildName(BasicFile* pFolder, int nIndex, WCHAR sChildName[MAX_PATH])
{
	ULONG n;

	if (!BasicFile_IsFolder(pFolder))
		return FALSE;

	n = BasicFolder_GetChildNum_Internal(pFolder);

	while(n--)
	{
		FolderEntry* pFolderEntry = &((FolderEntry*)pFolder->pData)[n];

		if (pFolderEntry->pFile != NULL)
		{
			if (nIndex == 0)
			{
				wcscpy_s(sChildName, MAX_PATH, pFolderEntry->sName);

				return TRUE;
			}
			else --nIndex;
		}
	}

	return FALSE;
}