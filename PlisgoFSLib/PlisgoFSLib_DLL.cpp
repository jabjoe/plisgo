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
#include "PlisgoVFS.h"


class HostFile : public PlisgoFSStringReadOnly
{
public:
	HostFile(const std::wstring& rsHostPath) : PlisgoFSStringReadOnly(rsHostPath)
	{
	}

	virtual bool				IsValid() const	{ return false; }
};


class HostFolder : public PlisgoFSFolder
{
public:
	HostFolder(	const std::wstring& rsHostPath,
				PlisgoToHostCBs*	pHostCBs) : m_sHostPath(rsHostPath),
											m_pHostCBs(pHostCBs)
	{}

	const std::wstring&			GetHostPath() const		{ return m_sHostPath; }

	virtual bool				IsValid() const	{ return false; }

	virtual bool				ForEachChild(EachChild& rEachChild) const
	{
		bool bResult = false;

		void* pHostFolderData = NULL;

		if (m_pHostCBs->OpenHostFolderCB(m_sHostPath.c_str(), &pHostFolderData, m_pHostCBs->pUserData))
		{
			bResult = true;

			while(bResult)
			{
				WCHAR sName[MAX_PATH];
				BOOL bIsFolder = FALSE;

				if (m_pHostCBs->NextHostFolderChildCB(pHostFolderData, sName, &bIsFolder, m_pHostCBs->pUserData))
				{
					IPtrPlisgoFSFile file = CreateChildNode(sName, bIsFolder);

					bResult = rEachChild.Do(sName, file);
				}
				else break;
			}

			m_pHostCBs->CloseHostFolderCB(pHostFolderData, m_pHostCBs->pUserData);
		}

		return bResult;
	}


	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sName) const
	{
		IPtrPlisgoFSFile result;

		void* pHostFolderData = NULL;

		if (m_pHostCBs->OpenHostFolderCB(m_sHostPath.c_str(), &pHostFolderData, m_pHostCBs->pUserData))
		{
			BOOL bIsFolder = FALSE;

			if (m_pHostCBs->QueryHostFolderChildCB(pHostFolderData, sName, &bIsFolder, m_pHostCBs->pUserData))
				result = CreateChildNode(sName, bIsFolder);

			m_pHostCBs->CloseHostFolderCB(pHostFolderData, m_pHostCBs->pUserData);
		}

		return result;
	}


	virtual int					Repath(LPCWSTR , LPCWSTR , bool , PlisgoFSFolder* )		{ return -ERROR_ACCESS_DENIED; }

private:

	IPtrPlisgoFSFile			CreateChildNode(LPCWSTR sName, BOOL bIsFolder) const
	{
		IPtrPlisgoFSFile result;

		std::wstring sFullName = m_sHostPath;
		sFullName+= L"\\";
		sFullName+= sName;

		if (bIsFolder)
			result.reset(new HostFolder(sFullName, m_pHostCBs));
		else
			result.reset(new HostFile(sFullName));

		return result;
	}

	std::wstring		m_sHostPath;
	PlisgoToHostCBs*	m_pHostCBs;
};


class HostFolderWithExtras : public HostFolder
{
public:
	HostFolderWithExtras(	const std::wstring& rsHostPath,
							PlisgoToHostCBs*	pHostCBs) : HostFolder(rsHostPath, pHostCBs)
	{
	}


	virtual bool				ForEachChild(EachChild& rEachChild) const
	{
		if (!m_Extras.ForEachFile(rEachChild))
			return false;

		return HostFolder::ForEachChild(rEachChild);
	}

	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sName) const
	{
		IPtrPlisgoFSFile result = m_Extras.GetFile(sName);

		if (result.get() != NULL)
			return result;

		return HostFolder::GetChild(sName);
	}

	virtual int					AddChild(LPCWSTR sName, IPtrPlisgoFSFile file)
	{
		IPtrPlisgoFSFile existing = HostFolder::GetChild(sName);

		if (existing.get() == NULL)
			return -ERROR_ACCESS_DENIED;

		m_Extras.AddFile(sName, file);

		return 0;
	}

private:
	PlisgoFSFileList	m_Extras;
};


static	bool	GetHostPath(IPtrPlisgoFSFile& rFile, std::wstring& rsResult)
{
	if (rFile.get() == NULL)
		return false;

	PlisgoFSFolder* pFolder = rFile->GetAsFolder();

	BOOL bCallResult = FALSE;

	if (pFolder != NULL)
	{
		HostFolder* pHostFolder = dynamic_cast<HostFolder*>(pFolder);

		if (pHostFolder == NULL)
			return false;

		rsResult = pHostFolder->GetHostPath();
	}
	else
	{
		HostFile* pHostFile = dynamic_cast<HostFile*>(rFile.get());

		if (pHostFile == NULL)
			return false;

		pHostFile->GetWideString(rsResult);
	}

	return true;
}



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


	virtual LPCWSTR	GetFFSName() const	{ return m_CBs.sFSName; }

	virtual bool IsShelled(IPtrPlisgoFSFile& rFile) const
	{
		std::wstring sHostPath;

		if (!GetHostPath(rFile, sHostPath))
			return false;

		return (m_CBs.IsShelledFolderCB(sHostPath.c_str(), m_CBs.pUserData))?true:false;
	}


	virtual bool GetColumnEntry(IPtrPlisgoFSFile& rFile, const int nColumnIndex, std::wstring& rsResult) const
	{
		std::wstring sHostPath;

		if (!GetHostPath(rFile, sHostPath))
			return false;

		WCHAR sBuffer[MAX_PATH];

		sBuffer[0] = L'\0';

		if (!m_CBs.GetColumnEntryCB(sHostPath.c_str(), (UINT)nColumnIndex, sBuffer, m_CBs.pUserData))
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

		if (!cb(rsFilePath.c_str(), &bIsList, &nList, sPath, &nIndex, m_CBs.pUserData))
			return false;

		if (bIsList)
			rResult.Set(nList, nIndex);
		else
			rResult.Set(sPath, nIndex);

		return true;
	}

	virtual bool GetOverlayIcon(IPtrPlisgoFSFile& rFile, IconLocation& rResult) const
	{
		std::wstring sHostPath;

		if (!GetHostPath(rFile, sHostPath))
			return false;

		return GetIcon(m_CBs.GetOverlayIconCB, sHostPath, rResult);
	}


	virtual bool GetCustomIcon(IPtrPlisgoFSFile& rFile, IconLocation& rResult) const
	{
		std::wstring sHostPath;

		if (!GetHostPath(rFile, sHostPath))
			return false;

		return GetIcon(m_CBs.GetCustomIconCB, sHostPath, rResult);
	}


	virtual bool GetThumbnail(IPtrPlisgoFSFile& rFile, std::wstring& rsExt, IPtrPlisgoFSFile& rThumbnailFile) const
	{
		std::wstring sHostPath;

		if (!GetHostPath(rFile, sHostPath))
			return false;

		WCHAR sPathBuffer[MAX_PATH];

		if (!m_CBs.GetThumbnailCB(sHostPath.c_str(), sPathBuffer, m_CBs.pUserData))
			return false;

		WCHAR* sExt = wcsrchr(sPathBuffer, L'.');

		if (sExt == NULL)
			return false;

		rsExt = sExt;

		rThumbnailFile = IPtrPlisgoFSFile(new PlisgoFSRealFile(sPathBuffer));

		return true;
	}
};





struct PlisgoFiles
{
	IPtrPlisgoVFS		VFS;
	PlisgoToHostCBs*	pHostCBs;
};



struct PlisgoFolder
{
	std::wstring							sMount;
	boost::shared_ptr<RootPlisgoFSFolder>	Plisgo;
	boost::shared_ptr<C_IShellInfoFetcher>	ShellInfoFetcher;
};



struct PlisgoVirtualFile
{
	PlisgoVFS::PlisgoFileHandle hFileHandle;
	std::wstring				sPath;
	IPtrPlisgoVFS				VFS;
};




class C_FileEvent : public FileEvent
{
public:

	C_FileEvent(PlisgoPathCB cb, void* pUserData)
	{
		assert(cb != NULL);

		m_CB = cb;
		m_pUserData = pUserData;
	}

	virtual bool Do(IPtrPlisgoFSFile& rFile)
	{
		std::wstring sPath;

		GetHostPath(rFile, sPath);

		return (m_CB(sPath.c_str(), m_pUserData))?true:false;
	}

	PlisgoPathCB	m_CB;
	void*			m_pUserData;
};


int		PlisgoFilesCreate(PlisgoFiles** ppPlisgoFiles, PlisgoToHostCBs* pHostCBs)
{
	if (ppPlisgoFiles == NULL)
		return -ERROR_BAD_ARGUMENTS;

	*ppPlisgoFiles = new PlisgoFiles;

	if (*ppPlisgoFiles == NULL)
		return -ERROR_NO_SYSTEM_RESOURCES;
	
	(*ppPlisgoFiles)->VFS = boost::make_shared<PlisgoVFS>(boost::make_shared<HostFolderWithExtras>(L"", pHostCBs));
	(*ppPlisgoFiles)->pHostCBs = pHostCBs;

	return 0;
}


int		PlisgoFilesDestroy(PlisgoFiles* pPlisgoFiles)
{
	if (pPlisgoFiles == NULL)
		return -ERROR_BAD_ARGUMENTS;

	delete pPlisgoFiles;

	return 0;
}






int		PlisgoFolderCreate(PlisgoFiles* pPlisgoFiles, PlisgoFolder** ppPlisgoFolder, LPCWSTR sFolder, PlisgoGUICBs* pCBs)
{
	if (pPlisgoFiles == NULL || ppPlisgoFolder == NULL || pCBs == NULL)
		return -ERROR_BAD_ARGUMENTS;

	IPtrPlisgoFSFile mountParent = pPlisgoFiles->VFS->TracePath(sFolder);

	if (mountParent.get() == NULL)
		return -ERROR_BAD_PATHNAME;

	PlisgoFSFolder* pFolder = mountParent->GetAsFolder();

	if (pFolder == NULL)
		return -ERROR_BAD_PATHNAME;

	IPtrPlisgoFSFile stub = pFolder->GetChild(L".plisgofs");

	if (stub.get() == NULL)
		return -ERROR_BAD_PATHNAME;

	stub = pFolder->GetChild(L"Desktop.ini");

	if (stub.get() == NULL)
		return -ERROR_BAD_PATHNAME;


	if (dynamic_cast<HostFolderWithExtras*>(mountParent.get()) == NULL)
	{
		mountParent = IPtrPlisgoFSFile(new HostFolderWithExtras(sFolder, pPlisgoFiles->pHostCBs));

		if (!pPlisgoFiles->VFS->AddMount(sFolder, mountParent))
			return -ERROR_BAD_PATHNAME;
	}
	
	*ppPlisgoFolder = new PlisgoFolder;

	if (*ppPlisgoFolder == NULL)
		return -ERROR_NO_SYSTEM_RESOURCES;

	C_IShellInfoFetcher* pIShellInfoFetcher = new C_IShellInfoFetcher(pCBs);

	(*ppPlisgoFolder)->ShellInfoFetcher.reset(pIShellInfoFetcher);
	(*ppPlisgoFolder)->sMount = sFolder;

	IPtrRootPlisgoFSFolder Plisgo = pIShellInfoFetcher->CreatePlisgoFolder(pPlisgoFiles->VFS, sFolder);

	if (Plisgo.get() == NULL || pIShellInfoFetcher == NULL)
	{
		delete *ppPlisgoFolder;
		*ppPlisgoFolder = NULL;

		return -ERROR_NO_SYSTEM_RESOURCES;
	}

	(*ppPlisgoFolder)->Plisgo = Plisgo;

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


int		PlisgoFolderDestroy(PlisgoFolder* pPlisgoFolder)
{
	if (pPlisgoFolder == NULL)
		return -ERROR_BAD_ARGUMENTS;

	pPlisgoFolder->Plisgo->GetVFS()->RemoveMount(pPlisgoFolder->sMount.c_str());

	return 0;
}




int		PlisgoFolderAddColumn(PlisgoFolder* pPlisgoFolder, LPCWSTR sColumnHeader)
{
	if (pPlisgoFolder == NULL || sColumnHeader == NULL)
		return -ERROR_BAD_ARGUMENTS;

	if (pPlisgoFolder->Plisgo->AddColumn(sColumnHeader))
		return 0;
	else
		return -ERROR_BAD_ARGUMENTS;
}


int		PlisgoFolderSetColumnType(PlisgoFolder* pPlisgoFolder, UINT nColumn, UINT nType)
{
	if (pPlisgoFolder == NULL)
		return -ERROR_BAD_ARGUMENTS;

	pPlisgoFolder->Plisgo->SetColumnType(nColumn, (RootPlisgoFSFolder::ColumnType)nType);

	return 0;
}


int		PlisgoFolderSetColumnAlignment(PlisgoFolder* pPlisgoFolder, UINT nColumn, int nAlignment)
{
	if (pPlisgoFolder == NULL)
		return -ERROR_BAD_ARGUMENTS;

	pPlisgoFolder->Plisgo->SetColumnAlignment(nColumn, (RootPlisgoFSFolder::ColumnAlignment)nAlignment);

	return 0;
}


int		PlisgoFolderAddIconsList(PlisgoFolder* pPlisgoFolder, LPCWSTR sImageFilePath)
{
	if (pPlisgoFolder == NULL || sImageFilePath == NULL)
		return -ERROR_BAD_ARGUMENTS;

	UINT nIconIndex = pPlisgoFolder->Plisgo->GetIconListNum();

	if (pPlisgoFolder->Plisgo->AddIcons(nIconIndex, sImageFilePath))
		return 0;
	else
		return -ERROR_BAD_ARGUMENTS;
}


int		PlisgoFolderAddCustomFolderIcon(PlisgoFolder* pPlisgoFolder,
										UINT nClosedIconList, UINT nClosedIconIndex,
										UINT nOpenIconList, UINT nOpenIconIndex)
{
	if (pPlisgoFolder == NULL)
		return -ERROR_BAD_ARGUMENTS;

	pPlisgoFolder->Plisgo->AddCustomFolderIcon(nClosedIconList, nClosedIconIndex, nOpenIconList, nOpenIconIndex);

	return 0;
}


int		PlisgoFolderAddCustomDefaultIcon(PlisgoFolder* pPlisgoFolder, UINT nList, UINT nIndex)
{
	if (pPlisgoFolder == NULL)
		return -ERROR_BAD_ARGUMENTS;

	pPlisgoFolder->Plisgo->AddCustomDefaultIcon(nList, nIndex);

	return 0;
}


int		PlisgoFolderAddCustomExtensionIcon(PlisgoFolder* pPlisgoFolder, LPCWSTR sExt, UINT nList, UINT nIndex)
{
	if (pPlisgoFolder == NULL || sExt == NULL)
		return -ERROR_BAD_ARGUMENTS;

	pPlisgoFolder->Plisgo->AddCustomExtensionIcon(sExt, nList, nIndex);

	return 0;
}


int		PlisgoFolderAddMenuItem(PlisgoFolder*	pPlisgoFolder,
								int*			pnResultIndex,
								LPCWSTR			sText,
								PlisgoPathCB	onClickCB,
								int				nParentMenu,
								PlisgoPathCB	isEnabledCB,
								UINT			nIconList,
								UINT			nIconIndex,
								void*			pCBUserData)
{
	if (pPlisgoFolder == NULL || pnResultIndex == NULL || sText == NULL)
		return -ERROR_BAD_ARGUMENTS;

	IPtrFileEvent onClickObj;
	IPtrFileEvent isEnabledObj;

	if (onClickCB != NULL)
		onClickObj.reset(new C_FileEvent(onClickCB, pCBUserData));

	if (isEnabledCB != NULL)
		isEnabledObj.reset(new C_FileEvent(isEnabledCB, pCBUserData));

	*pnResultIndex = pPlisgoFolder->Plisgo->AddMenu(sText, onClickObj, nParentMenu, nIconList, nIconIndex, isEnabledObj);

	return 0;
}


int		PlisgoFolderAddMenuSeparatorItem(PlisgoFolder*	pPlisgoFolder, int nParentMenu)
{
	if (pPlisgoFolder == NULL)
		return -ERROR_BAD_ARGUMENTS;

	pPlisgoFolder->Plisgo->AddSeparator(nParentMenu);

	return 0;
}



/*
****************************************************************************************
*/


static bool HostFileToSkip(IPtrPlisgoFSFile& rFile)
{
	PlisgoFSFile* pFile = rFile.get();

	assert(pFile != NULL);

	return (dynamic_cast<HostFile*>(pFile) != NULL || dynamic_cast<HostFolder*>(pFile) != NULL);
}


int		PlisgoVirtualFileOpen(	PlisgoFiles*		pPlisgoFiles,
								LPCWSTR				sPath,
								PlisgoVirtualFile**	ppFile,
								DWORD				nDesiredAccess,
								DWORD				nShareMode,
								DWORD				nCreationDisposition,
								DWORD				nFlagsAndAttributes)
{
	if (pPlisgoFiles == NULL || sPath == NULL || ppFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	PlisgoVFS::PlisgoFileHandle hFileHandle;

	int nError = pPlisgoFiles->VFS->Open(	hFileHandle,
											sPath,
											nDesiredAccess,
											nShareMode,
											nCreationDisposition,
											nFlagsAndAttributes);

	if (nError == 0)
	{
		IPtrPlisgoFSFile file = pPlisgoFiles->VFS->GetPlisgoFSFile(hFileHandle);
		
		if (HostFileToSkip(file))
		{
			pPlisgoFiles->VFS->Close(hFileHandle);

			return -ERROR_FILE_NOT_FOUND;
		}

		*ppFile = new PlisgoVirtualFile;

		if (*ppFile == NULL)
		{
			pPlisgoFiles->VFS->Close(hFileHandle);

			return -ERROR_NO_SYSTEM_RESOURCES;
		}

		(*ppFile)->hFileHandle = hFileHandle;
		(*ppFile)->sPath	= sPath;
		(*ppFile)->VFS		= pPlisgoFiles->VFS;

		return 0;
	}

	return nError;
}



int		PlisgoVirtualFileClose(PlisgoVirtualFile* pFile)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	pFile->VFS->Close(pFile->hFileHandle);

	delete pFile;

	return 0;
}


int		PlisgoVirtualGetFileInfo(PlisgoVirtualFile* pFile, PlisgoFileInfo* pInfo)
{
	if (pFile == NULL || pInfo == NULL)
		return -ERROR_BAD_ARGUMENTS;

	BY_HANDLE_FILE_INFORMATION info;

	int nError = pFile->VFS->GetHandleInfo(pFile->hFileHandle, &info);

	if (nError != 0)
		return nError;

	pInfo->nAttr		= info.dwFileAttributes;

	pInfo->nCreation	= *(ULONGLONG*)&info.ftCreationTime;
	pInfo->nLastAccess	= *(ULONGLONG*)&info.ftLastAccessTime;
	pInfo->nLastWrite	= *(ULONGLONG*)&info.ftLastWriteTime;

	wcscpy_s(pInfo->sName, MAX_PATH, GetNameFromPath(pFile->sPath));

	LARGE_INTEGER temp;

	temp.HighPart = info.nFileSizeHigh;
	temp.LowPart = info.nFileSizeLow;

	pInfo->nSize = temp.QuadPart;

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

	return pFile->VFS->Read(pFile->hFileHandle, pBuffer, nNumberOfBytesToRead, pnNumberOfBytesRead, nOffset);
}


int		PlisgoVirtualFileWrite(	PlisgoVirtualFile*	pFile,
								LPCVOID				pBuffer,
								DWORD				nNumberOfBytesToWrite,
								LPDWORD				pnNumberOfBytesWritten,
								LONGLONG			nOffset)
{
	if (pFile == NULL || pBuffer == NULL)
		return -ERROR_BAD_ARGUMENTS;
	
	return pFile->VFS->Write(pFile->hFileHandle, pBuffer, nNumberOfBytesToWrite, pnNumberOfBytesWritten, nOffset);
}


int		PlisgoVirtualFileLock(PlisgoVirtualFile* pFile, LONGLONG nByteOffset, LONGLONG nByteLength)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->VFS->LockFile(pFile->hFileHandle, nByteOffset, nByteOffset);
}


int		PlisgoVirtualFileUnlock(PlisgoVirtualFile* pFile, LONGLONG nByteOffset, LONGLONG nByteLength)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->VFS->UnlockFile(pFile->hFileHandle, nByteOffset, nByteOffset);
}


int		PlisgoVirtualFileFlushBuffers(PlisgoVirtualFile* pFile)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->VFS->FlushBuffers(pFile->hFileHandle);
}


int		PlisgoVirtualFileFlushSetEndOf(PlisgoVirtualFile* pFile, LONGLONG nEndPos)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->VFS->SetEndOfFile(pFile->hFileHandle, nEndPos);
}


int		PlisgoVirtualFileFlushSetTimes(PlisgoVirtualFile* pFile, ULONGLONG* pnCreation, ULONGLONG* pnLastAccess, ULONGLONG* pnLastWrite)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->VFS->SetFileTimes(pFile->hFileHandle, (FILETIME*)pnCreation, (FILETIME*)pnLastAccess, (FILETIME*)pnLastWrite);
}


int		PlisgoVirtualFileFlushSetAttributes(PlisgoVirtualFile* pFile, DWORD nAttr)
{
	if (pFile == NULL)
		return -ERROR_BAD_ARGUMENTS;

	return pFile->VFS->SetAttributes(pFile->hFileHandle, nAttr);
}


class ForEachChildRedirect : public PlisgoFSFolder::EachChild
{
public:

	ForEachChildRedirect(PlisgoVirtualFileForEachChildCB cb, void* pData)
	{
		m_cb	= cb;
		m_pData = pData;
	}

	virtual bool Do(LPCWSTR sName, IPtrPlisgoFSFile file)
	{
		if (HostFileToSkip(file))
			return true;

		PlisgoFileInfo info = {0};

		FILETIME Creation;
		FILETIME LastAccess;
		FILETIME LastWrite;

		file->GetFileTimes(Creation, LastAccess, LastWrite);

		info.nCreation		= *(ULONGLONG*)&Creation;
		info.nLastAccess	= *(ULONGLONG*)&LastAccess;
		info.nLastWrite		= *(ULONGLONG*)&LastWrite;

		wcscpy_s(info.sName, MAX_PATH, sName);
		info.nAttr			= file->GetAttributes();
		info.nSize			= file->GetSize();

		return (m_cb(&info, m_pData))?true:false;
	}

	PlisgoVirtualFileForEachChildCB		m_cb;
	void*								m_pData;
};


int		PlisgoVirtualFileForEachChild(PlisgoVirtualFile* pFile, PlisgoVirtualFileForEachChildCB cb, void* pData)
{
	if (pFile == NULL || cb == NULL)
		return -ERROR_BAD_ARGUMENTS;

	ForEachChildRedirect cbObj(cb, pData);
	
	return pFile->VFS->ForEachChild(pFile->hFileHandle, cbObj);
}

