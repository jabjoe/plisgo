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

#include "PlisgoFSLib.h"
#include "PlisgoFSHiddenFolders.h"
#include "PlisgoFSLib_DLL.h"



class C_IShellInfoFetcher : public IShellInfoFetcher
{
	PlisgoGUICBs m_CBs;

public:

	C_IShellInfoFetcher(PlisgoGUICBs* pCBs)
	{
		if (pCBs != NULL)
			m_CBs = *pCBs;
		else
			ZeroMemory(&m_CBs, sizeof(PlisgoGUICBs));
	}

	virtual bool ReadShelled(const std::wstring& rsFolderPath, BasicFolder* pResult) const
	{
		if (m_CBs.FindInShelledFolderCB == NULL)
			return false;
		
		if (pResult == NULL)
		{
			//It's just a query to see if it's a shelled folder

			return (m_CBs.FindInShelledFolderCB(rsFolderPath.c_str(), 0, NULL, 0, NULL, m_CBs.pUserData) == TRUE);
		}
		else
		{
			FileNameBuffer	sName;
			BOOL			nIsFolder = FALSE;
			UINT			nRequestIndex = 0;

			sName[0] = L'\0';

			if (!m_CBs.FindInShelledFolderCB(rsFolderPath.c_str(), nRequestIndex, sName, MAX_PATH, &nIsFolder, m_CBs.pUserData))
				return false;

			if (sName[0] == L'\0')
				return false;
			
			do
			{
				const size_t nIndex = pResult->size();

				pResult->resize(nIndex+1);

				BasicFileInfo& rEntry = pResult->at(nIndex);

				rEntry.bIsFolder	= (nIsFolder)?true:false;

				memcpy_s(rEntry.sName, MAX_PATH, sName, MAX_PATH);

				++nRequestIndex;
			}
			while(m_CBs.FindInShelledFolderCB(rsFolderPath.c_str(), nRequestIndex, sName, MAX_PATH, &nIsFolder, m_CBs.pUserData));
		
			return true;
		}
	}

	virtual bool GetColumnEntry(const std::wstring& rsFilePath, const int nColumnIndex, std::wstring& rsResult) const
	{
		if (m_CBs.GetColumnEntryCB == NULL)
			return false;

		WCHAR sBuffer[MAX_PATH];

		if (!m_CBs.GetColumnEntryCB(rsFilePath.c_str(), (const UINT)nColumnIndex, sBuffer, MAX_PATH, m_CBs.pUserData))
			return false;

		rsResult = sBuffer;

		return true;
	}

	bool GetIcon(PlisgoGetIconCB cb, const std::wstring& rsFilePath, IconLocation& rResult) const
	{
		if (cb == NULL)
			return false;

		BOOL bIsList;
		UINT nList;
		UINT nIndex;
		WCHAR sPath[MAX_PATH];

		if (!cb(rsFilePath.c_str(), &bIsList, &nList, sPath, MAX_PATH, &nIndex, m_CBs.pUserData))
			return false;

		if (bIsList)
			rResult.Set(nList, nIndex);
		else
			rResult.Set(sPath, nIndex);

		return true;
	}

	virtual bool GetOverlayIcon(const std::wstring& rsFilePath, IconLocation& rResult) const
	{
		return GetIcon(m_CBs.GetOverlayIconCB, rsFilePath, rResult);
	}

	virtual bool GetCustomIcon(const std::wstring& rsFilePath, IconLocation& rResult) const
	{
		return GetIcon(m_CBs.GetCustomIconCB, rsFilePath, rResult);
	}

	virtual bool GetThumbnail(const std::wstring& rsFilePath, LPCWSTR sExt, IPtrPlisgoFSFile& rThumbnailFile) const
	{
		if (m_CBs.GetThumbnailCB == NULL || sExt == NULL)
			return false;

		WCHAR sBuffer[MAX_PATH];

		if (!m_CBs.GetThumbnailCB(rsFilePath.c_str(), sExt, sBuffer, MAX_PATH, m_CBs.pUserData))
			return false;

		rThumbnailFile = IPtrPlisgoFSFile(new PlisgoFSRedirectionFile(sBuffer));

		return true;
	}
};





struct PlisgoFiles
{
	PlisgoFiles() : VFS(boost::make_shared<PlisgoFSStorageFolder>())
	{
	}

	PlisgoVFS								VFS;
	boost::shared_ptr<RootPlisgoFSFolder>	Plisgo;
	boost::shared_ptr<C_IShellInfoFetcher>	ShellInfoFetcher;
};



struct PlisgoVirtualFile
{
	IPtrPlisgoFSFile file;
	ULONGLONG		 nInstanceData;
};



class CallForEachChild : public PlisgoFSFolder::EachChild
{
public:

	CallForEachChild(PlisgoVirtualFileChildCB cb,  void* pData)
	{
		assert(cb != NULL);

		m_cb		= cb;
		m_pData		= pData;
		m_nError	= 0;

	}

	virtual bool Do(LPCWSTR sName, IPtrPlisgoFSFile file)
	{
		FILETIME Creation;
		FILETIME LastAccess;
		FILETIME LastWrite;

		if(!file->GetFileTimes(Creation, LastAccess, LastWrite))
		{
			m_nError = -ERROR_BAD_PATHNAME;

			return false;
		}

		m_nError = m_cb(sName,
						file->GetAttributes(),
						file->GetSize(),
						*(ULONGLONG*)&Creation,
						*(ULONGLONG*)&LastAccess,
						*(ULONGLONG*)&LastWrite,
						m_pData);

		return (m_nError == 0);
	}


	int	GetError() const { return m_nError; }


private:
	PlisgoVirtualFileChildCB	m_cb;
	void*						m_pData;
	int							m_nError;
};



class C_StringEvent : public StringEvent
{
public:

	C_StringEvent(PlisgoPathCB cb, void* pUserData)
	{
		assert(cb != NULL);

		m_CB = cb;
		m_pUserData = pUserData;
	}

	virtual bool Do(LPCWSTR sPath)
	{
		return (m_CB(sPath, m_pUserData))?true:false;
	}

	PlisgoPathCB	m_CB;
	void*			m_pUserData;
};




int		PlisgoFilesCreate(PlisgoFiles** ppPlisgoFiles, LPCWSTR sFSName, PlisgoGUICBs* pCBs)
{
	if (ppPlisgoFiles == NULL)
		return -ERROR_BAD_ARGUMENTS;

	*ppPlisgoFiles = new PlisgoFiles;

	if (*ppPlisgoFiles == NULL)
		return -ERROR_NO_SYSTEM_RESOURCES;

	if (pCBs != NULL)
		(*ppPlisgoFiles)->ShellInfoFetcher.reset(new C_IShellInfoFetcher(pCBs));


	boost::shared_ptr<RootPlisgoFSFolder> Plisgo =  boost::make_shared<RootPlisgoFSFolder>(sFSName, (*ppPlisgoFiles)->ShellInfoFetcher.get());

	if (Plisgo.get() == NULL)
	{
		delete *ppPlisgoFiles;
		*ppPlisgoFiles = NULL;

		return -ERROR_NO_SYSTEM_RESOURCES;
	}

	(*ppPlisgoFiles)->Plisgo = Plisgo;

	IPtrPlisgoFSFolder root = (*ppPlisgoFiles)->VFS.GetRoot();

	root->AddChild(L".plisgofs", Plisgo);
	root->AddChild(L"Desktop.ini", GetPlisgoDesktopIniFile());

	if (pCBs != NULL)
	{
		if (pCBs->GetThumbnailCB != NULL)
			Plisgo->EnableThumbnails();
		
		if (pCBs->GetCustomIconCB != NULL)
			Plisgo->EnableCustomIcons();

		if (pCBs->GetOverlayIconCB != NULL)
			Plisgo->EnableOverlays();

	}

	return 0;
}


int		PlisgoFilesDestroy(PlisgoFiles* pPlisgoFiles)
{
	if (pPlisgoFiles == NULL)
		return -ERROR_BAD_ARGUMENTS;

	delete pPlisgoFiles;

	return 0;
}


int		PlisgoFilesAddColumn(PlisgoFiles* pPlisgoFiles, LPCWSTR sColumnHeader)
{
	if (pPlisgoFiles == NULL || sColumnHeader == NULL)
		return -ERROR_BAD_ARGUMENTS;

	if (pPlisgoFiles->Plisgo->AddColumn(sColumnHeader))
		return 0;
	else
		return -ERROR_BAD_ARGUMENTS;
}


int		PlisgoFilesSetColumnType(PlisgoFiles* pPlisgoFiles, UINT nColumn, UINT nType)
{
	if (pPlisgoFiles == NULL)
		return -ERROR_BAD_ARGUMENTS;

	pPlisgoFiles->Plisgo->SetColumnType(nColumn, (RootPlisgoFSFolder::ColumnType)nType);

	return 0;
}


int		PlisgoFilesSetColumnAlignment(PlisgoFiles* pPlisgoFiles, UINT nColumn, int nAlignment)
{
	if (pPlisgoFiles == NULL)
		return -ERROR_BAD_ARGUMENTS;

	pPlisgoFiles->Plisgo->SetColumnAlignment(nColumn, (RootPlisgoFSFolder::ColumnAlignment)nAlignment);

	return 0;
}


int		PlisgoFilesAddIconsList(PlisgoFiles* pPlisgoFiles, LPCWSTR sImageFilePath)
{
	if (pPlisgoFiles == NULL || sImageFilePath == NULL)
		return -ERROR_BAD_ARGUMENTS;

	UINT nIconIndex = pPlisgoFiles->Plisgo->GetIconListNum();

	if (pPlisgoFiles->Plisgo->AddIcons(nIconIndex, sImageFilePath))
		return 0;
	else
		return -ERROR_BAD_ARGUMENTS;
}


int		PlisgoFilesAddCustomFolderIcon(	PlisgoFiles* pPlisgoFiles,
										UINT nClosedIconList, UINT nClosedIconIndex,
										UINT nOpenIconList, UINT nOpenIconIndex)
{
	if (pPlisgoFiles == NULL)
		return -ERROR_BAD_ARGUMENTS;

	pPlisgoFiles->Plisgo->AddCustomFolderIcon(nClosedIconList, nClosedIconIndex, nOpenIconList, nOpenIconIndex);

	return 0;
}


int		PlisgoFilesAddCustomDefaultIcon(PlisgoFiles* pPlisgoFiles, UINT nList, UINT nIndex)
{
	if (pPlisgoFiles == NULL)
		return -ERROR_BAD_ARGUMENTS;

	pPlisgoFiles->Plisgo->AddCustomDefaultIcon(nList, nIndex);

	return 0;
}


int		PlisgoFilesAddCustomExtensionIcon(PlisgoFiles* pPlisgoFiles, LPCWSTR sExt, UINT nList, UINT nIndex)
{
	if (pPlisgoFiles == NULL || sExt == NULL)
		return -ERROR_BAD_ARGUMENTS;

	pPlisgoFiles->Plisgo->AddCustomExtensionIcon(sExt, nList, nIndex);

	return 0;
}


int		PlisgoFilesAddMenuItem(	PlisgoFiles*	pPlisgoFiles,
								int*			pnResultIndex,
								LPCWSTR			sText,
								PlisgoPathCB	onClickCB,
								int				nParentMenu,
								PlisgoPathCB	isEnabledCB,
								UINT			nIconList,
								UINT			nIconIndex,
								void*			pCBUserData)
{
	if (pPlisgoFiles == NULL || pnResultIndex == NULL || sText == NULL)
		return -ERROR_BAD_ARGUMENTS;

	IPtrStringEvent onClickObj;
	IPtrStringEvent isEnabledObj;

	if (onClickCB != NULL)
		onClickObj.reset(new C_StringEvent(onClickCB, pCBUserData));

	if (isEnabledCB != NULL)
		isEnabledObj.reset(new C_StringEvent(isEnabledCB, pCBUserData));

	*pnResultIndex = pPlisgoFiles->Plisgo->AddMenu(sText, onClickObj, nParentMenu, isEnabledObj, nIconList, nIconIndex);

	return 0;
}


int		PlisgoFilesAddMenuSeparatorItem(PlisgoFiles* pPlisgoFiles, int nParentMenu)
{
	if (pPlisgoFiles == NULL)
		return -ERROR_BAD_ARGUMENTS;

	pPlisgoFiles->Plisgo->AddSeparator(nParentMenu);

	return 0;
}


int		PlisgoFilesForRootFiles(PlisgoFiles* pPlisgoFiles, PlisgoVirtualFileChildCB cb, void* pData)
{
	if (pPlisgoFiles == NULL || cb == NULL)
		return -ERROR_BAD_ARGUMENTS;

	CallForEachChild cbObj(cb, pData);

	pPlisgoFiles->VFS.GetRoot()->ForEachChild(cbObj);

	return cbObj.GetError();
}




/*
****************************************************************************************
*/



int		PlisgoVirtualFileOpen(	PlisgoFiles*		pPlisgoFiles,
								LPCWSTR				sPath,
								BOOL*				pbRootVirtualPath,
								PlisgoVirtualFile**	ppFile,
								DWORD				nDesiredAccess,
								DWORD				nShareMode,
								DWORD				nCreationDisposition,
								DWORD				nFlagsAndAttributes)
{
	if (pPlisgoFiles == NULL || sPath == NULL || ppFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	IPtrPlisgoFSFile parent;

	IPtrPlisgoFSFile file = pPlisgoFiles->VFS.TracePath(sPath, &parent);

	if (pbRootVirtualPath != NULL)
		*pbRootVirtualPath = (parent.get() != NULL);

	if (file.get() == NULL)
		return -ERROR_BAD_PATHNAME;


	ULONGLONG		 nInstanceData;

	int nError = file->Open(nDesiredAccess, nShareMode, nCreationDisposition, nFlagsAndAttributes, &nInstanceData);

	if (nError != 0)
		return nError;

	*ppFile = new PlisgoVirtualFile;

	if (*ppFile == NULL)
	{
		nError = file->Close(&nInstanceData);

		if (nError != 0)
			return nError;

		return -ERROR_NO_SYSTEM_RESOURCES;
	}

	(*ppFile)->file				= file;
	(*ppFile)->nInstanceData	= nInstanceData;

	return 0;
}



int		PlisgoVirtualFileClose(PlisgoVirtualFile* pFile)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	int nError = pFile->file->Close(&pFile->nInstanceData);

	if (nError != 0)
		return nError;

	delete pFile;

	return 0;
}



int		PlisgoVirtualFileGetAttributes(PlisgoVirtualFile* pFile, DWORD* pnAttr)
{
	if (pFile == NULL || pnAttr == NULL)
		return -ERROR_BAD_ARGUMENTS;

	*pnAttr = pFile->file->GetAttributes();

	return 0;
}


int		PlisgoVirtualFileGetTimes(	PlisgoVirtualFile*	pFile,
									ULONGLONG*			pnCreation,
									ULONGLONG*			pnLastAccess,
									ULONGLONG*			pnLastWrite)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	FILETIME Creation;
	FILETIME LastAccess;
	FILETIME LastWrite;

	int nError = pFile->file->GetFileTimes(Creation, LastAccess, LastWrite);

	if (nError != 0)
		return nError;

	if (pnCreation != NULL)
		*pnCreation = *(ULONGLONG*)&Creation;

	if (pnLastAccess != NULL)
		*pnLastAccess = *(ULONGLONG*)&LastAccess;

	if (pnLastWrite != NULL)
		*pnLastWrite = *(ULONGLONG*)&LastWrite;

	return 0;
}


int		PlisgoVirtualFileGetSize(PlisgoVirtualFile* pFile, ULONGLONG* pnSize)
{
	if (pFile == NULL || pnSize == NULL)
		return -ERROR_BAD_ARGUMENTS;

	*pnSize = pFile->file->GetSize();

	return 0;
}


int		PlisgoVirtualFileRead(	PlisgoVirtualFile*	pFile,
								LPVOID				pBuffer,
								DWORD				nNumberOfBytesToRead,
								LPDWORD				pnNumberOfBytesRead,
								LONGLONG			nOffset)
{
	if (pFile == NULL || pBuffer == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->Read(pBuffer, nNumberOfBytesToRead, pnNumberOfBytesRead, nOffset, &pFile->nInstanceData);
}


int		PlisgoVirtualFileWrite(	PlisgoVirtualFile*	pFile,
								LPCVOID				pBuffer,
								DWORD				nNumberOfBytesToWrite,
								LPDWORD				pnNumberOfBytesWritten,
								LONGLONG			nOffset)
{
	if (pFile == NULL || pBuffer == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->Write(pBuffer, nNumberOfBytesToWrite, pnNumberOfBytesWritten, nOffset, &pFile->nInstanceData);
}


int		PlisgoVirtualFileLock(PlisgoVirtualFile* pFile, LONGLONG nByteOffset, LONGLONG nByteLength)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->LockFile( nByteOffset, nByteOffset, &pFile->nInstanceData);
}


int		PlisgoVirtualFileUnlock(PlisgoVirtualFile* pFile, LONGLONG nByteOffset, LONGLONG nByteLength)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->UnlockFile( nByteOffset, nByteOffset, &pFile->nInstanceData);
}


int		PlisgoVirtualFileFlushBuffers(PlisgoVirtualFile* pFile)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->FlushBuffers( &pFile->nInstanceData);
}


int		PlisgoVirtualFileFlushSetEndOf(PlisgoVirtualFile* pFile, LONGLONG nEndPos)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->SetEndOfFile(nEndPos, &pFile->nInstanceData);
}


int		PlisgoVirtualFileFlushSetTimes(PlisgoVirtualFile* pFile, ULONGLONG* pnCreation, ULONGLONG* pnLastAccess, ULONGLONG* pnLastWrite)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->SetFileTimes((FILETIME*)pnCreation, (FILETIME*)pnLastAccess, (FILETIME*)pnLastWrite, &pFile->nInstanceData);
}


int		PlisgoVirtualFileFlushSetAttributes(PlisgoVirtualFile* pFile, DWORD nAttr)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->file->SetAttributes(nAttr, &pFile->nInstanceData);
}


int		PlisgoVirtualFileForEachChild(PlisgoVirtualFile* pFile, PlisgoVirtualFileChildCB cb, void* pData)
{
	if (pFile == NULL || cb == NULL)
		return -ERROR_BAD_ARGUMENTS;

	PlisgoFSFolder* pFolder = pFile->file->GetAsFolder();

	if (pFolder == NULL)
		return -ERROR_ACCESS_DENIED; //Not a folder

	CallForEachChild cbObj(cb, pData);

	pFolder->ForEachChild(cbObj);

	return cbObj.GetError();
}



