
#ifndef __PLISGOFSLIB_DLL__
#define __PLISGOFSLIB_DLL__

#ifdef _DLL
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#ifdef EUROSAFEDLL_EXPORTS
#define PLISGOCALL __declspec(dllexport)
#else
#define PLISGOCALL __declspec(dllimport)
#endif


typedef int (*PlisgoVirtualFileChildCB)(const WCHAR*	sName,
										DWORD			nAttr,
										ULONGLONG		nSize,
										ULONGLONG		nCreation,
										ULONGLONG		nLastAccess,
										ULONGLONG		nLastWrite,
										void*			pUserData);

typedef struct PlisgoFiles PlisgoFiles;
typedef struct PlisgoVirtualFile PlisgoVirtualFile;



PLISGOCALL	int		PlisgoFilesCreate(PlisgoFiles** ppPlisgoFiles, const WCHAR* sFSName);
PLISGOCALL	int		PlisgoFilesDestroy(PlisgoFiles* pPlisgoFiles);

PLISGOCALL	int		PlisgoFilesForRootFiles(PlisgoFiles* pPlisgoFiles, PlisgoVirtualFileChildCB cb, void* pData);





PLISGOCALL	int		PlisgoVirtualFileOpen(	PlisgoFiles*		pPlisgoFiles,
											const WCHAR*		sPath,
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

#endif //__PLISGOFSLIB_DLL__

