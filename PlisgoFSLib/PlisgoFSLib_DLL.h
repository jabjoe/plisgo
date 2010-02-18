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





// These calls are used to query the host filesystem about file and folders Plisgo is to provide a shell for
typedef BOOL (*PlisgoOpenHostFolderCB)(LPCWSTR sPath, void** pHostFolderData, void* pData);
typedef BOOL (*PlisgoNextHostFolderChildCB)(void* pHostFolderData, WCHAR sChildName[MAX_PATH], BOOL* pbIsFolder, void* pData);
typedef BOOL (*PlisgoQueryHostFolderChildCB)(void* pHostFolderData, LPCWSTR sChildName, BOOL* pbIsFolder, void* pData);
typedef BOOL (*PlisgoCloseHostFolderCB)(void* pHostFolderData, void* pData);


/*
	This structure is used to provide Plisgo with callbacks to query about the host filesystem
*/
struct PlisgoToHostCBs
{
	void*							pUserData; //Use to store what ever you need, is passed into each callback
	PlisgoOpenHostFolderCB			OpenHostFolderCB;
	PlisgoNextHostFolderChildCB		NextHostFolderChildCB;
	PlisgoQueryHostFolderChildCB	QueryHostFolderChildCB;
	PlisgoCloseHostFolderCB			CloseHostFolderCB;
};
typedef struct PlisgoToHostCBs PlisgoToHostCBs;

typedef struct PlisgoFiles PlisgoFiles;

PLISGOCALL	int		PlisgoFilesCreate(PlisgoFiles** ppPlisgoFiles, PlisgoToHostCBs* pHostCBs);
PLISGOCALL	int		PlisgoFilesDestroy(PlisgoFiles* pPlisgoFiles);



/*
	Callback to be implimented by Plisgo shell implimentor
*/

// Call types used to obtain shell information about host filesystem
typedef BOOL (*PlisgoIsShelledFolderCB)(LPCWSTR sFolderPath, void* pData);
typedef BOOL (*PlisgoGetColumnEntryCB)(LPCWSTR sFile, UINT nColumn, WCHAR sBuffer[MAX_PATH], void* pData);
typedef BOOL (*PlisgoGetIconCB)(LPCWSTR sFile, BOOL* pbUsesList, UINT* pnList, WCHAR sPathBuffer[MAX_PATH], UINT* pnEntryIndex, void* pData);
typedef BOOL (*PlisgoGetThumbnailCB)(LPCWSTR sFile, WCHAR sPathBuffer[MAX_PATH], void* pData);

//Called for menu enabled test and when menu item clicked.
typedef BOOL (*PlisgoPathCB)(LPCWSTR sFile, void* pData); 


/*
	This structure is used to provide Plisgo with the callbacks to create the shell
*/
struct PlisgoGUICBs
{
	void*							pUserData; //Use to store what ever you need, is passed into each callback

	WCHAR							sFSName[MAX_PATH];

	PlisgoIsShelledFolderCB			IsShelledFolderCB;
	PlisgoGetColumnEntryCB			GetColumnEntryCB;
	PlisgoGetIconCB					GetOverlayIconCB;
	PlisgoGetIconCB					GetCustomIconCB;
	PlisgoGetThumbnailCB			GetThumbnailCB;
};
typedef struct PlisgoGUICBs PlisgoGUICBs;



typedef struct PlisgoFolder PlisgoFolder;


PLISGOCALL	int		PlisgoFolderCreate(PlisgoFiles* pPlisgoFiles, PlisgoFolder** ppPlisgoFolder, LPCWSTR sFolder, PlisgoGUICBs* pCBs);
PLISGOCALL	int		PlisgoFolderDestroy(PlisgoFolder* pPlisgoFolder);


//Custom columns
PLISGOCALL	int		PlisgoFolderAddColumn(PlisgoFolder* pPlisgoFolder, LPCWSTR sColumnHeader);
//0 is text, 1 is int, 2 is float
PLISGOCALL	int		PlisgoFolderSetColumnType(PlisgoFolder* pPlisgoFolder, UINT nColumn, UINT nType);
//-1 is left, 0 is center, 1 is right
PLISGOCALL	int		PlisgoFolderSetColumnAlignment(PlisgoFolder* pPlisgoFolder, UINT nColumn, int nAlignment);


//Custom icons
PLISGOCALL	int		PlisgoFolderAddIconsList(PlisgoFolder* pPlisgoFolder, LPCWSTR sImageFilePath);

PLISGOCALL	int		PlisgoFolderAddCustomFolderIcon(	PlisgoFolder* pPlisgoFolder,
														UINT nClosedIconList, UINT nClosedIconIndex,
														UINT nOpenIconList, UINT nOpenIconIndex);

PLISGOCALL	int		PlisgoFolderAddCustomDefaultIcon(PlisgoFolder* pPlisgoFolder, UINT nList, UINT nIndex);

PLISGOCALL	int		PlisgoFolderAddCustomExtensionIcon(PlisgoFolder* pPlisgoFolder, LPCWSTR sExt, UINT nList, UINT nIndex);


//Custom menus
PLISGOCALL	int		PlisgoFolderAddMenuItem(PlisgoFolder*	pPlisgoFolder,
											int*			pnResultIndex,
											LPCWSTR			sText,
											PlisgoPathCB	onClickCB,
											int				nParentMenu, //-1 for none
											PlisgoPathCB	isEnabledCB,
											UINT			nIconList, //-1 for none
											UINT			nIconIndex, //-1 for none
											void*			pCBUserData);

PLISGOCALL int		PlisgoFolderAddMenuSeparatorItem(PlisgoFolder* pPlisgoFolder, int nParentMenu); //-1 for none





/*
	Open a PlisgoVirtualFile for file operations
	call this from you virtual file system if you don't get -ERROR_BAD_PATHNAME result and PlisgoVirtualFile is set
*/
typedef struct PlisgoVirtualFile PlisgoVirtualFile;

struct PlisgoFileInfo
{
	WCHAR		sName[MAX_PATH];
	DWORD		nAttr;
	ULONGLONG	nSize;
	//ULONGLONG is 64bit and is the value of a NTFS FILETIME
	ULONGLONG	nCreation;
	ULONGLONG	nLastAccess;
	ULONGLONG	nLastWrite;
};
typedef struct PlisgoFileInfo PlisgoFileInfo;



PLISGOCALL	int		PlisgoVirtualFileOpen(	PlisgoFiles*		pPlisgoFiles,
											LPCWSTR				sPath,
											PlisgoVirtualFile**	ppFile,
											DWORD				nDesiredAccess,
											DWORD				nShareMode,
											DWORD				nCreationDisposition,
											DWORD				nFlagsAndAttributes);

PLISGOCALL	int		PlisgoVirtualFileClose(PlisgoVirtualFile* pFile);

PLISGOCALL	int		PlisgoVirtualGetFileInfo(PlisgoVirtualFile* pFile, PlisgoFileInfo* pInfo);

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


typedef BOOL (*PlisgoVirtualFileForEachChildCB)(PlisgoFileInfo* pPlisgoFileInfo, void* pData);

PLISGOCALL	int		PlisgoVirtualFileForEachChild(PlisgoVirtualFile* pFile, PlisgoVirtualFileForEachChildCB cb, void* pData);


#ifdef __cplusplus
}
#endif //__cplusplus
#endif //_DLL

