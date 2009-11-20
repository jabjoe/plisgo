/*
	Copyright 2009 Eurocom Entertainment Software Limited

    This file is part of PlisgoFSLib.

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

#ifndef __PLISGOFSLIB_DLL__
#define __PLISGOFSLIB_DLL__

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#ifdef EUROSAFEDLL_EXPORTS
#define PLISGOCALL __declspec(dllexport)
#else
#define PLISGOCALL __declspec(dllimport)
#endif

//ULONGLONG is 64bit and is the value of a NTFS FILETIME

typedef int (*PlisgoVirtualFileChildCB)(LPCWSTR		sName,
										DWORD		nAttr,
										ULONGLONG	nSize,
										ULONGLONG	nCreation,
										ULONGLONG	nLastAccess,
										ULONGLONG	nLastWrite,
										void*		pUserData);

typedef struct PlisgoFiles PlisgoFiles;
typedef struct PlisgoVirtualFile PlisgoVirtualFile;

//A NULL sNameBuffer means just a query if it's a shelled folder or not.
typedef BOOL (*PlisgoFindInShelledFolderCB)(LPCWSTR sFolderPath, UINT nRequestIndex, WCHAR* sNameBuffer, UINT nBufferSize, BOOL* pIsFolder, void* pData);
typedef BOOL (*PlisgoGetColumnEntryCB)(LPCWSTR sFile, UINT nColumn, WCHAR* sBuffer, UINT nBufferSize, void* pData);
typedef BOOL (*PlisgoGetIconCB)(LPCWSTR sFile, BOOL* pbUsesList, UINT* pnList, WCHAR* sPathBuffer, UINT nBufferSize, UINT* pnEntryIndex, void* pData);
typedef BOOL (*PlisgoGetThumbnailCB)(LPCWSTR sFile, LPCWSTR sThumbnailExt, WCHAR* sPathBuffer, UINT nBufferSize, void* pData);


typedef BOOL (*PlisgoPathCB)(LPCWSTR sFile, void* pData);


struct PlisgoGUICBs
{
	void*						pUserData;
	PlisgoFindInShelledFolderCB	FindInShelledFolderCB;
	PlisgoGetColumnEntryCB		GetColumnEntryCB;
	PlisgoGetIconCB				GetOverlayIconCB;
	PlisgoGetIconCB				GetCustomIconCB;
	PlisgoGetThumbnailCB		GetThumbnailCB;
};
typedef struct PlisgoGUICBs PlisgoGUICBs;


PLISGOCALL	int		PlisgoFilesCreate(PlisgoFiles** ppPlisgoFiles, LPCWSTR sFSName, PlisgoGUICBs* pCBs);
PLISGOCALL	int		PlisgoFilesDestroy(PlisgoFiles* pPlisgoFiles);

//Custom columns
PLISGOCALL	int		PlisgoFilesAddColumn(PlisgoFiles* pPlisgoFiles, LPCWSTR sColumnHeader);
//0 is text, 1 is int, 2 is float
PLISGOCALL	int		PlisgoFilesSetColumnType(PlisgoFiles* pPlisgoFiles, UINT nColumn, UINT nType);
//-1 is left, 0 is center, 1 is right
PLISGOCALL	int		PlisgoFilesSetColumnAlignment(PlisgoFiles* pPlisgoFiles, UINT nColumn, int nAlignment);


//Custom icons
PLISGOCALL	int		PlisgoFilesAddIconsList(PlisgoFiles* pPlisgoFiles, LPCWSTR sImageFilePath);

PLISGOCALL	int		PlisgoFilesAddCustomFolderIcon(	PlisgoFiles* pPlisgoFiles,
													UINT nClosedIconList, UINT nClosedIconIndex,
													UINT nOpenIconList, UINT nOpenIconIndex);

PLISGOCALL	int		PlisgoFilesAddCustomDefaultIcon(PlisgoFiles* pPlisgoFiles, UINT nList, UINT nIndex);

PLISGOCALL	int		PlisgoFilesAddCustomExtensionIcon(PlisgoFiles* pPlisgoFiles, LPCWSTR sExt, UINT nList, UINT nIndex);


//Custom menus
PLISGOCALL	int		PlisgoFilesAddMenuItem(	PlisgoFiles*	pPlisgoFiles,
											int*			pnResultIndex,
											LPCWSTR			sText,
											PlisgoPathCB	onClickCB,
											int				nParentMenu, //-1 for none
											PlisgoPathCB	isEnabledCB,
											UINT			nIconList, //-1 for none
											UINT			nIconIndex, //-1 for none
											void*			pCBUserData);

PLISGOCALL int		PlisgoFilesAddMenuSeparatorItem(PlisgoFiles* pPlisgoFiles, int nParentMenu); //-1 for none


//Call for .plisgo folder and desktop.ini files

PLISGOCALL	int		PlisgoFilesForRootFiles(PlisgoFiles* pPlisgoFiles, PlisgoVirtualFileChildCB cb, void* pData);




/*
	Open a PlisgoVirtualFile for file operations
	call this from you virtual file system if you don't get -ERROR_BAD_PATHNAME result and PlisgoVirtualFile is set
*/

PLISGOCALL	int		PlisgoVirtualFileOpen(	PlisgoFiles*		pPlisgoFiles,
											LPCWSTR				sPath,
											BOOL*				pbRootVirtualPath,
											PlisgoVirtualFile**	ppFile,
											DWORD				nDesiredAccess,
											DWORD				nShareMode,
											DWORD				nCreationDisposition,
											DWORD				nFlagsAndAttributes);

PLISGOCALL	int		PlisgoVirtualFileClose(PlisgoVirtualFile* pFile);

PLISGOCALL	int		PlisgoVirtualFileGetAttributes(PlisgoVirtualFile* pFile, DWORD* pnAttr);

PLISGOCALL	int		PlisgoVirtualFileGetTimes(PlisgoVirtualFile*	pFile,
											  ULONGLONG*			pnCreation,
											  ULONGLONG*			pnLastAccess,
											  ULONGLONG*			pnLastWrite);

PLISGOCALL	int		PlisgoVirtualFileGetSize(PlisgoVirtualFile* pFile, ULONGLONG* pnSize);


PLISGOCALL	int		PlisgoVirtualFileRead(PlisgoVirtualFile* pFile, LPVOID		pBuffer,
																	DWORD		nNumberOfBytesToRead,
																	LPDWORD		pnNumberOfBytesRead,
																	LONGLONG	nOffset);

PLISGOCALL	int		PlisgoVirtualFileWrite(PlisgoVirtualFile* pFile,LPCVOID		pBuffer,
																	DWORD		nNumberOfBytesToWrite,
																	LPDWORD		pnNumberOfBytesWritten,
																	LONGLONG	nOffset);

PLISGOCALL	int		PlisgoVirtualFileLock(PlisgoVirtualFile* pFile, LONGLONG nByteOffset, LONGLONG nByteLength);

PLISGOCALL	int		PlisgoVirtualFileUnlock(PlisgoVirtualFile* pFile, LONGLONG nByteOffset, LONGLONG nByteLength);

PLISGOCALL	int		PlisgoVirtualFileFlushBuffers(PlisgoVirtualFile* pFile);

PLISGOCALL	int		PlisgoVirtualFileFlushSetEndOf(PlisgoVirtualFile* pFile, LONGLONG nEndPos);


PLISGOCALL	int		PlisgoVirtualFileFlushSetTimes(PlisgoVirtualFile* pFile, ULONGLONG* pnCreation, ULONGLONG* pnLastAccess, ULONGLONG* pnLastWrite);

PLISGOCALL	int		PlisgoVirtualFileFlushSetAttributes(PlisgoVirtualFile* pFile, DWORD nAttr);

PLISGOCALL	int		PlisgoVirtualFileForEachChild(PlisgoVirtualFile* pFile, PlisgoVirtualFileChildCB cb, void* pUserData);


#ifdef __cplusplus
}
#endif //__cplusplus
#endif //_DLL

