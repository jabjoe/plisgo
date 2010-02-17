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

#ifndef __BASICFILESYSTEM__
#define __BASICFILESYSTEM__

struct BasicStats
{
	ULONGLONG	nCreation;
	ULONGLONG	nLastAccess;
	ULONGLONG	nLastWrite;
	ULONGLONG	nSize;
	DWORD		nAttr;
};
typedef struct BasicStats BasicStats;

typedef struct BasicFile BasicFile;


// file functions

extern UINT			GetTotalBasicFileNum();


extern BasicFile*	BasicFile_Create();

extern void			BasicFile_GetStats(BasicFile* pFile, BasicStats* pStats);

extern void			BasicFile_Release(BasicFile* pFile);

extern void			BasicFile_WriteData(BasicFile* pFile, UINT nPos, void* pData, UINT nDataSize);
extern UINT			BasicFile_ReadData(BasicFile* pFile, UINT nPos, void* pData, UINT nDataSize);

// Folder functions

extern BOOL			BasicFile_SetAsFolder(BasicFile* pFile);
extern BOOL			BasicFile_IsFolder(BasicFile* pFile);

extern void			BasicFolder_AddChild(BasicFile* pFolder, LPCWSTR sName, BasicFile* pFile);
extern void			BasicFolder_RemoveChild(BasicFile* pFolder, LPCWSTR sName);

extern BasicFile*	BasicFolder_GetChild(BasicFile* pFolder, LPCWSTR sName);

extern BasicFile*	BasicFolder_GetChildByIndex(BasicFile* pFolder, int nIndex);
extern BOOL			BasicFolder_GetChildName(BasicFile* pFolder, int nIndex, WCHAR sChildName[MAX_PATH]);

extern BasicFile*	BasicFolder_WalkPath(BasicFile* pStart, LPCWSTR sPath);

typedef BOOL (*BasicFolder_ForEachChildCB)(LPCWSTR sName, BasicFile* pFile, void* pData);

extern void			BasicFolder_ForEachChild(BasicFile* pFolder, BasicFolder_ForEachChildCB cb, void* pData);

#endif //__BASICFILESYSTEM__