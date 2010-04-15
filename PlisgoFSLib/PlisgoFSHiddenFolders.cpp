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
#include "PlisgoFSMenuItem.h"

#include <Unknwn.h>
#include <gdiplus.h>



//Create a plisgo folder set up with this IShellInfoFetcher
IPtrRootPlisgoFSFolder	IShellInfoFetcher::CreatePlisgoFolder(IPtrPlisgoVFS& rVFS, const std::wstring& rsPath, bool bDoMounts )
{
	assert(rVFS.get() != NULL);

	IPtrRootPlisgoFSFolder plisgoFS(new RootPlisgoFSFolder(rsPath, GetFFSName(), rVFS, shared_from_this()));

	assert(plisgoFS.get() != NULL);

	if (bDoMounts)
		DoMounts(plisgoFS, rVFS, rsPath);

	return plisgoFS;
}


void	IShellInfoFetcher::DoMounts(IPtrRootPlisgoFSFolder plisgoFS, IPtrPlisgoVFS& rVFS, const std::wstring& rsPath)
{
	assert(plisgoFS.get() != NULL);

	std::wstring sMount = rsPath;

	sMount+= L"\\.plisgofs";

	IPtrPlisgoFSFile parent;
	
	IPtrPlisgoFSFile mount = rVFS->TracePath(sMount.c_str(), &parent);

	if (parent.get() == NULL)
	{
		//Er wtf, you gave me a invalid path!
		return;
	}

	PlisgoFSFolder* pFolder = parent->GetAsFolder();

	assert(pFolder != NULL); //WTF

	if (mount.get() == NULL)
	{
		pFolder->CreateChild(mount, L".plisgofs", FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY);

		if (mount.get() == NULL)
			return; //Er wtf, it isn't there, and you won't let me put it there?
	}


	rVFS->AddMount(sMount.c_str(), plisgoFS);

	mount = pFolder->GetChild(L"Desktop.ini");

	if (mount.get() == NULL)
	{
		pFolder->CreateChild(mount, L"Desktop.ini", FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY);

		if (mount.get() == NULL)
		{
			rVFS->RemoveMount(sMount.c_str());

			return; //Er wtf, it isn't there, and you won't let me put it there?
		}
	}

	rVFS->AddMount((rsPath + L"\\Desktop.ini").c_str() , GetPlisgoDesktopIniFile());
}

/*
*********************************************************************
			File/Folder classes
*********************************************************************
*/

class StubShellInfoFolder : public PlisgoFSFolder
{
public:
	StubShellInfoFolder(const std::wstring& rsSubjectPath, RootPlisgoFSFolder* pRoot)
	{
		assert(pRoot != NULL);

		m_sSubjectPath	= rsSubjectPath;

		if (m_sSubjectPath.length() == 0)
			m_sSubjectPath = L"\\";
		else if (m_sSubjectPath[m_sSubjectPath.length()-1] != L'\\')
			m_sSubjectPath += L"\\";

		m_pRoot			= pRoot;
	}

	class DirectEachChild : public PlisgoFSFolder::EachChild
	{
	public:

		DirectEachChild(const StubShellInfoFolder* pObj, PlisgoFSFolder::EachChild& rEachChild) : m_rEachChild(rEachChild)
		{
			m_pObj = pObj;
		}

		virtual bool Do(LPCWSTR sName, IPtrPlisgoFSFile file)
		{
			IPtrPlisgoFSFile shellFile;

			std::wstring sShellName;

			m_pObj->GetEntryFile(shellFile, sShellName, file, sName);

			if (shellFile.get() == NULL)
				return true;

			return m_rEachChild.Do(sShellName.c_str(), shellFile);
		}

	private:
		const StubShellInfoFolder*	m_pObj;
		PlisgoFSFolder::EachChild&	m_rEachChild;
	};


	virtual bool				GetEntryFile(	IPtrPlisgoFSFile& rDstFile, std::wstring& rsDstName,
												IPtrPlisgoFSFile& rSrcFile, const std::wstring& rsSrcName) const = 0;

	virtual bool				GetEntryRealName(std::wstring& rsDstName, const std::wstring& rsSrcName) const
	{
		rsDstName = rsSrcName;

		return true;
	}

	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sNameUnknownCase) const
	{
		std::wstring sShellName = sNameUnknownCase;
		std::wstring sRealName;

		if (!GetEntryRealName(sRealName, sShellName))
			return IPtrPlisgoFSFile();

		std::wstring sRealFile	= m_sSubjectPath;

		sRealFile+= sRealName;

		IPtrPlisgoFSFile file = m_pRoot->GetVFS()->TracePath(sRealFile.c_str());

		if (file.get() == NULL)
			return file;

		IPtrPlisgoFSFile result;

		GetEntryFile(result, sShellName, file, sRealName);

		return result;
	}


	virtual bool				ForEachChild(EachChild& rEachChild) const
	{
		DirectEachChild cb(this, rEachChild);

		const int nError = m_pRoot->GetVFS()->ForEachChild(m_sSubjectPath.c_str(), cb);

		return (nError == 0);
	}

protected:

	std::wstring				m_sSubjectPath;
	RootPlisgoFSFolder*			m_pRoot;
};





class ColumnFolder : public StubShellInfoFolder
{
public:

	ColumnFolder(	const std::wstring& rsSubjectPath,
					RootPlisgoFSFolder* pRoot,
					const int nColumn) : StubShellInfoFolder(rsSubjectPath, pRoot)
	{
		m_nColumnIndex = nColumn;
	}

	virtual bool		GetEntryFile(	IPtrPlisgoFSFile& rDstFile, std::wstring& rsDstName,
										IPtrPlisgoFSFile& rSrcFile, const std::wstring& rsSrcName) const
	{
		rsDstName = rsSrcName;

		std::wstring sText;

		if (!m_pRoot->GetShellInfoFetcher()->GetColumnEntry(rSrcFile, m_nColumnIndex, sText))
			return false;

		PlisgoFSStringReadOnly* pStrFile = new PlisgoFSStringReadOnly(sText);

		assert(pStrFile != NULL);

		pStrFile->SetVolatile(true);

		rDstFile.reset(pStrFile);

		return true;
	}

private:
	 int m_nColumnIndex;
};


class OverlayIconsFolder : public StubShellInfoFolder
{
public:
	OverlayIconsFolder(	const std::wstring& rsSubjectPath,
						RootPlisgoFSFolder* pRoot)	: StubShellInfoFolder(rsSubjectPath, pRoot) {}


	virtual bool		GetEntryFile(	IPtrPlisgoFSFile& rDstFile, std::wstring& rsDstName,
										IPtrPlisgoFSFile& rSrcFile, const std::wstring& rsSrcName) const
	{
		rsDstName = rsSrcName;
		IconLocation icon;

		if (!m_pRoot->GetShellInfoFetcher()->GetOverlayIcon(rSrcFile, icon))
			return false;

		std::string sText;

		if (!icon.GetText(sText))
			return false;

		PlisgoFSStringReadOnly* pStrFile = new PlisgoFSStringReadOnly(sText);

		assert(pStrFile != NULL);

		pStrFile->SetVolatile(true);

		rDstFile.reset(pStrFile);

		return true;
	}
};


class CustomIconsFolder : public StubShellInfoFolder
{
public:
	CustomIconsFolder(	const std::wstring& rsSubjectPath,
						RootPlisgoFSFolder* pRoot)	: StubShellInfoFolder(rsSubjectPath, pRoot) {}

	virtual bool			GetEntryFile(	IPtrPlisgoFSFile& rDstFile, std::wstring& rsDstName,
											IPtrPlisgoFSFile& rSrcFile, const std::wstring& rsSrcName) const
	{
		rsDstName = rsSrcName;
		IconLocation icon;

		if (!m_pRoot->GetShellInfoFetcher()->GetCustomIcon(rSrcFile, icon))
			return false;

		std::string sText;

		if (!icon.GetText(sText))
			return false;

		PlisgoFSStringReadOnly* pStrFile = new PlisgoFSStringReadOnly(sText);

		assert(pStrFile != NULL);

		pStrFile->SetVolatile(true);

		rDstFile.reset(pStrFile);

		return true;
	}
};


class VolatileEncapsulatedFile : public PlisgoFSEncapsulatedFile
{
public:
	VolatileEncapsulatedFile(IPtrPlisgoFSFile& rFile) : PlisgoFSEncapsulatedFile(rFile) {}

	virtual bool	IsValid() const		{ return false; }
};


class ThumbnailsFolder : public StubShellInfoFolder
{
public:

	ThumbnailsFolder(	const std::wstring& rsSubjectPath,
						RootPlisgoFSFolder* pRoot)	: StubShellInfoFolder(rsSubjectPath, pRoot) {}


	virtual bool			GetEntryFile(	IPtrPlisgoFSFile& rDstFile, std::wstring& rsDstName,
											IPtrPlisgoFSFile& rSrcFile, const std::wstring& rsSrcName) const
	{
		rsDstName = rsSrcName;

		std::wstring sExt;

		if (!m_pRoot->GetShellInfoFetcher()->GetThumbnail(rSrcFile, sExt, rDstFile))
			return false;

		if (sExt.length() && sExt[0] != L'.')
			rsDstName += L".";

		rsDstName += sExt;

		rDstFile.reset(new VolatileEncapsulatedFile(rDstFile));

		return true;
	}

	virtual bool			GetEntryRealName(std::wstring& rsDstName, const std::wstring& rsSrcName) const
	{
		rsDstName = rsSrcName;

		const size_t nDot = rsSrcName.rfind(L'.');

		if (nDot == -1)
			return false;

		rsDstName.resize(nDot);

		return true;
	}
};



bool		IconLocation::GetText(std::string& rsResult) const
{
	if (m_nIndex == -1)
		return false;

	if (IsFromFile())
	{
		rsResult.assign(m_sFile.begin(),m_sFile.end());
		rsResult+= ";";
		rsResult+= ( boost::format("%1%") %m_nIndex).str();
	}
	else
	{
		if (m_nList == -1)
			return false;

		rsResult = ( boost::format("%1% : %2%") %m_nList %m_nIndex).str();
	}

	return true;
}


class ShellInfoFolder : public StubShellInfoFolder
{
public:

	ShellInfoFolder(	const std::wstring& rsSubjectPath,
						RootPlisgoFSFolder* pRoot) : StubShellInfoFolder(rsSubjectPath, pRoot)
	{}

	virtual bool			GetEntryFile(	IPtrPlisgoFSFile& rDstFile, std::wstring& rsDstName,
											IPtrPlisgoFSFile& rSrcFile, const std::wstring& rsSrcName) const
	{
		if (rSrcFile.get() == NULL)
			return false;

		if (rSrcFile->GetAsFolder() == NULL)
			return false;

		if (!m_pRoot->GetShellInfoFetcher()->IsShelled(rSrcFile))
			return false;

		rDstFile	= CreateChildShellInfoFolder(rsSrcName);
		rsDstName	= rsSrcName;

		return true;
	}


	virtual bool				ForEachChild(EachChild& rEachChild) const
	{
		InitVirtualFolder();

		//This shouldn't be called unless the user has for some crazy reason browsed in!

		if (!m_virtualChildren.ForEachFile(rEachChild))
			return false;
		
		return StubShellInfoFolder::ForEachChild(rEachChild);
	}


	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sNameUnknownCase) const
	{
		InitVirtualFolder();

		IPtrPlisgoFSFile result = m_virtualChildren.GetFile(sNameUnknownCase);

		if (result.get() != NULL)
			return result;

		return StubShellInfoFolder::GetChild(sNameUnknownCase);
	}

protected:

	void						InitVirtualFolder() const
	{
		//The whole const thing is because of cacheing, it's trying to avoid de-consting other than when init needs doing

		//It doens't matter about if this isn't 100% thread safe, because the duplicate will just overwrite.

		if (m_virtualChildren.GetLength() != 0)
			return; //Done already

		if (m_pRoot->HasOverlays())
			const_cast<ShellInfoFolder*>(this)->m_virtualChildren.AddFile(L".overlay_icons", IPtrPlisgoFSFile(new OverlayIconsFolder(m_sSubjectPath, m_pRoot)));

		if (m_pRoot->HasCustomIcons())
			const_cast<ShellInfoFolder*>(this)->m_virtualChildren.AddFile(L".custom_icons", IPtrPlisgoFSFile(new CustomIconsFolder(m_sSubjectPath, m_pRoot)));

		if (m_pRoot->HasThumbnails())
			const_cast<ShellInfoFolder*>(this)->m_virtualChildren.AddFile(L".thumbnails", IPtrPlisgoFSFile(new ThumbnailsFolder(m_sSubjectPath, m_pRoot)));
		
		const UINT nColumnNum = m_pRoot->GetColumnNum();

		for(UINT n = 0; n < nColumnNum; ++n)
			const_cast<ShellInfoFolder*>(this)->m_virtualChildren.AddFile(	(boost::wformat(L".column_%1%") %n).str().c_str(),
																			IPtrPlisgoFSFile(CreateColumnFolder((int)n)));
	}


	IPtrPlisgoFSFile			CreateChildShellInfoFolder(const std::wstring& rsName) const
	{
		return IPtrPlisgoFSFile( new ShellInfoFolder( m_sSubjectPath + rsName, m_pRoot));
	}

	IPtrPlisgoFSFile			CreateColumnFolder(const int nColumn) const
	{
		return IPtrPlisgoFSFile( new ColumnFolder(m_sSubjectPath, m_pRoot, nColumn));
	}


	PlisgoFSFileList			m_virtualChildren;
};


/*
***************************************************************
				RootPlisgoFSFolder
***************************************************************
*/


RootPlisgoFSFolder::RootPlisgoFSFolder(const std::wstring& rsPath, LPCWSTR sFSName, IPtrPlisgoVFS& rVFS, IPtrIShellInfoFetcher ShellInfoFetcher)
{
	assert(sFSName != NULL);

	boost::format fmt = boost::format("%1%") %PLISGO_APIVERSION;

	m_IShellInfoFetcher = ShellInfoFetcher;

	AddChild(L".fsname", IPtrPlisgoFSFile(new PlisgoFSStringReadOnly( sFSName)));
	AddChild(L".version", IPtrPlisgoFSFile(new PlisgoFSStringReadOnly(fmt.str())));

	m_nColumnNum	= 0;
	m_nIconListsNum = 0;
	m_nRootMenuNum	= 0;

	m_bEnableThumbnails		= false;
	m_bEnableCustomIcons	= false;
	m_bEnableOverlays		= false;
	m_bHasCustomDefaultIcon	= false;
	m_bHasCustomFolderIcons = false;
	
	m_sPath	= rsPath;
	m_VFS	= rVFS;

	ZeroMemory(&m_DisabledStandardColumn, sizeof(m_DisabledStandardColumn));
}


	
UINT				RootPlisgoFSFolder::GetColumnNum() const			{ boost::shared_lock<boost::shared_mutex> lock(m_Mutex); return m_nColumnNum; }
UINT				RootPlisgoFSFolder::GetIconListNum() const			{ boost::shared_lock<boost::shared_mutex> lock(m_Mutex); return m_nIconListsNum; }
bool				RootPlisgoFSFolder::HasThumbnails() const			{ boost::shared_lock<boost::shared_mutex> lock(m_Mutex); return m_bEnableThumbnails; }
bool				RootPlisgoFSFolder::HasCustomIcons() const			{ boost::shared_lock<boost::shared_mutex> lock(m_Mutex); return m_bEnableCustomIcons; }
bool				RootPlisgoFSFolder::HasOverlays() const				{ boost::shared_lock<boost::shared_mutex> lock(m_Mutex); return m_bEnableOverlays; }
bool				RootPlisgoFSFolder::HasCustomDefaultIcon() const	{ boost::shared_lock<boost::shared_mutex> lock(m_Mutex); return m_bHasCustomDefaultIcon; }
bool				RootPlisgoFSFolder::HasCustomFolderIcons() const	{ boost::shared_lock<boost::shared_mutex> lock(m_Mutex); return m_bHasCustomFolderIcons; }


bool				RootPlisgoFSFolder::AddIcons(HINSTANCE hExeHandle, int nListIndex, LPCWSTR sName, LPCWSTR sType, LPCWSTR sExt)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	bool bResult = false;

	HRSRC hRes = FindResourceW(hExeHandle, sName, sType);

	if (hRes != NULL)
	{
		DWORD nSize = SizeofResource(NULL, hRes);

		HGLOBAL hReallyStaticMem = LoadResource(hExeHandle, hRes);

		if (hReallyStaticMem != NULL)
		{
			PVOID pData = LockResource(hReallyStaticMem);

			if (pData != NULL)
			{
				IStream* pStream = NULL;

				//The HGLOBAL from LoadResource isn't a real one, and CreateStreamOnHGlobal need a real one.
				HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, nSize);

				void* pBuffer = ::GlobalLock(hGlobal);

				CopyMemory(pBuffer, pData, nSize);

				CreateStreamOnHGlobal(hGlobal, FALSE, &pStream);

				if (pStream != NULL)
				{
					Gdiplus::GdiplusStartupInput	GDiplusStartupInputData;
					ULONG_PTR						nDiplusToken;

					Gdiplus::GdiplusStartup(&nDiplusToken, &GDiplusStartupInputData, NULL);

					Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);

					if (pBitmap != NULL)
					{
						UINT nHeight = pBitmap->GetHeight();

						delete pBitmap;

						boost::wformat nameFmt = boost::wformat(L".icons_%1%_%2%.%3%") %nListIndex %nHeight %sExt;

						if (GetChild(nameFmt.str().c_str()).get() == NULL)
						{
							AddChild(nameFmt.str().c_str(), IPtrPlisgoFSFile(new PlisgoFSDataFile((BYTE*)pData, nSize )));

							if ((UINT)nListIndex >= m_nIconListsNum)
								m_nIconListsNum = nListIndex+1;

							bResult = true;
						}
					}

					Gdiplus::GdiplusShutdown(nDiplusToken);

					pStream->Release();
				}

				::GlobalUnlock(hGlobal);
				::GlobalFree(hGlobal);
			}
		}
	}

	return bResult;
}


bool				RootPlisgoFSFolder::AddIcons(int nListIndex, const std::wstring& sFilename)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (GetFileAttributesW(sFilename.c_str()) == INVALID_FILE_ATTRIBUTES)
		return false;

	int nResult = -1;

	Gdiplus::GdiplusStartupInput	GDiplusStartupInputData;
	ULONG_PTR						nDiplusToken;

	Gdiplus::GdiplusStartup(&nDiplusToken, &GDiplusStartupInputData, NULL);

	Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromFile(sFilename.c_str(), TRUE);

	bool bResult = false;

	if (pBitmap != NULL)
	{
		UINT nHeight = pBitmap->GetHeight();

		delete pBitmap;

		size_t nExtPos = sFilename.rfind(L".");

		boost::wformat nameFmt = boost::wformat(L".icons_%1%_%2%%3%") %nListIndex %nHeight %sFilename.substr(nExtPos);

		if (GetChild(nameFmt.str().c_str()).get() == NULL)
		{
			AddChild(nameFmt.str().c_str(), IPtrPlisgoFSFile(new PlisgoFSRealFile(sFilename)));

			if ((UINT)nListIndex >= m_nIconListsNum)
				m_nIconListsNum = nListIndex+1;

			bResult = true;
		}
	}

	Gdiplus::GdiplusShutdown(nDiplusToken);

	return bResult;
}



class FindMenuCallObj : public PlisgoFSFolder::EachChild
{
public:
	FindMenuCallObj(int& rnMenu) : m_rnMenu(rnMenu) {}
	
	void operator = (FindMenuCallObj&)	{}

	virtual bool Do(LPCWSTR, IPtrPlisgoFSFile file)
	{
		if (file->GetAttributes() & FILE_ATTRIBUTE_DIRECTORY)
		{
			--m_rnMenu;

			if (m_rnMenu == 0)
			{
				m_result = file;
				return false;
			}

			PlisgoFSFolder* pFolder = file->GetAsFolder();

			if (pFolder == NULL)
				return false;
		
			if (!pFolder->ForEachChild(*this))
				return false;
		}

		return true;
	}	

	IPtrPlisgoFSFile	m_result;
	int&				m_rnMenu;
};



IPtrPlisgoFSFile	RootPlisgoFSFolder::FindMenu(int nMenu)
{
	if (nMenu == -1)
		return IPtrPlisgoFSFile();

	for(UINT n = 0; n < m_nRootMenuNum; ++n)
	{
		std::wstring  sMenuChild = (boost::wformat(L".menu_%1%") %n).str();

		IPtrPlisgoFSFile child = GetChild(sMenuChild.c_str());

		if (child->GetAttributes() & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (nMenu == 0)
				return child;

			--nMenu;

			PlisgoFSFolder* pFolder = child->GetAsFolder();

			if (pFolder == NULL)
				return IPtrPlisgoFSFile();

			FindMenuCallObj finder(nMenu);

			if (!pFolder->ForEachChild(finder))
				return finder.m_result;
		}
	}

	return IPtrPlisgoFSFile();
}


class FolderCountCallObj : public PlisgoFSFolder::EachChild
{
public:
	FolderCountCallObj() { m_nCount = 0; }
	
	virtual bool Do(LPCWSTR, IPtrPlisgoFSFile file)
	{
		if (file->GetAttributes() & FILE_ATTRIBUTE_DIRECTORY)
			++m_nCount;

		return true;
	}

	int		m_nCount;
};


int					RootPlisgoFSFolder::AddMenu(LPCWSTR			sText,
												IPtrFileEvent	onClickEvent,
												int				nParentMenu,
												int				nIconList,
												int				nIconIndex, 
												IPtrFileEvent	enabledEvent)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	assert(nIconList < (int)m_nIconListsNum);

	IPtrPlisgoFSFile parentMenu = FindMenu(nParentMenu);

	int nResult = -1;

	if (parentMenu.get() != NULL)
	{		
		PlisgoFSMenuItem* pMenuFolder = static_cast<PlisgoFSMenuItem*>(parentMenu.get());

		FolderCountCallObj	counter;

		pMenuFolder->ForEachChild(counter);

		nResult = counter.m_nCount;

		std::wstring sMenuName = (boost::wformat(L".menu_%1%") %nResult).str();

		pMenuFolder->AddChild(sMenuName.c_str(), IPtrPlisgoFSFile(new PlisgoFSMenuItem(this, onClickEvent, enabledEvent, sText, nIconList, nIconIndex)));
	}
	else
	{
		std::wstring sMenuName = (boost::wformat(L".menu_%1%") %m_nRootMenuNum).str();

		nResult = (int)m_nRootMenuNum;

		++m_nRootMenuNum;

		AddChild(sMenuName.c_str(), IPtrPlisgoFSFile(new PlisgoFSMenuItem(this, onClickEvent, enabledEvent, sText, nIconList, nIconIndex)));
	}

	return nResult;
}


bool				RootPlisgoFSFolder::AddColumn(std::wstring sHeader)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (m_IShellInfoFetcher.get() == NULL)
		return false;

	boost::wformat headerFmt = boost::wformat(L".column_header_%1%") %m_nColumnNum;

	AddChild(headerFmt.str().c_str(), IPtrPlisgoFSFile(new PlisgoFSStringReadOnly(sHeader)));

	++m_nColumnNum;

	EnableCustomShell();

	return true;
}


bool				RootPlisgoFSFolder::SetColumnAlignment(UINT nColumnIndex, ColumnAlignment eAlignment)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (nColumnIndex >= m_nColumnNum)
		return false;

	std::wstring sName = (boost::wformat(L".column_alignment_%1%") %nColumnIndex).str();

	if (GetChild(sName.c_str()).get() != NULL)
		return false;

	if (eAlignment == CENTER)
		return true;

	AddChild(sName.c_str(), IPtrPlisgoFSFile(new PlisgoFSStringReadOnly((eAlignment == LEFT)?L"l":L"r")));

	return true;
}


bool				RootPlisgoFSFolder::SetColumnType(UINT nColumnIndex, ColumnType eType)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (nColumnIndex >= m_nColumnNum)
		return false;

	std::wstring sName = (boost::wformat(L".column_type_%1%") %nColumnIndex).str();

	if (GetChild(sName.c_str()).get() != NULL)
		return false;

	if (eType == TEXT)
		return true;

	AddChild(sName.c_str(), IPtrPlisgoFSFile(new PlisgoFSStringReadOnly((eType == INT)?L"i":L"f")));

	return true;
}


bool				RootPlisgoFSFolder::SetColumnDefaultWidth(UINT nColumnIndex, int nWidth)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (nColumnIndex >= m_nColumnNum)
		return false;

	if (nWidth < 0)
		return false;

	std::wstring sName = (boost::wformat(L".column_width_%1%") %nColumnIndex).str();

	if (GetChild(sName.c_str()).get() != NULL)
		return false;

	char sBuffer[MAX_PATH];

	sprintf_s(sBuffer, MAX_PATH, "%i", nWidth);

	AddChild(sName.c_str(), IPtrPlisgoFSFile(new PlisgoFSStringReadOnly(sBuffer)));

	return true;
}


bool				RootPlisgoFSFolder::EnableThumbnails()
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (m_IShellInfoFetcher.get() == NULL)
		return false;

	m_bEnableThumbnails = true;
	
	EnableCustomShell();

	return true;
}


bool				RootPlisgoFSFolder::EnableCustomIcons()
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (m_IShellInfoFetcher.get() == NULL)
		return false;

	m_bEnableCustomIcons = true;
	
	EnableCustomShell();

	return true;
}


bool				RootPlisgoFSFolder::EnableOverlays()
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (m_IShellInfoFetcher.get() == NULL)
		return false;

	m_bEnableOverlays = true;

	EnableCustomShell();
	
	return true;
}


void				RootPlisgoFSFolder::EnableCustomShell()
{
	if (GetChild(L".shellinfo").get() == NULL)
		AddChild(L".shellinfo", IPtrPlisgoFSFile(new ShellInfoFolder(m_sPath, this)));
}


IPtrPlisgoFSFile	RootPlisgoFSFolder::GetCustomTypeIconsFolder()
{
	IPtrPlisgoFSFile result = GetChild(L".type_icons");

	if (result == NULL)
	{
		result.reset(new PlisgoFSStorageFolder());

		assert(result.get() != NULL);

		AddChild(L".type_icons", result);
	}
	else
	{
		assert(result->GetAttributes() & FILE_ATTRIBUTE_DIRECTORY);
	}

	return result;
}


void				RootPlisgoFSFolder::AddCustomFolderIcon(int	nClosedIconList,int nClosedIconIndex,
															int	nOpenIconList,	int nOpenIconIndex)
{
	AddCustomExtensionIcon(L".folder_icon_closed",	nClosedIconList,nClosedIconIndex);
	AddCustomExtensionIcon(L".folder_icon_open",	nOpenIconList,	nOpenIconIndex);

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	m_bHasCustomFolderIcons = true;
}


void				RootPlisgoFSFolder::AddCustomDefaultIcon(int nList, int nIndex)
{
	AddCustomExtensionIcon(L".default_file",	nList, nIndex);

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	m_bHasCustomDefaultIcon = true;
}


void				RootPlisgoFSFolder::AddCustomExtensionIcon(LPCWSTR sExt, int nList, int nIndex)
{
	if (sExt == NULL || nList < 0 || nIndex < 0 || sExt[1] == L'\0' || (sExt[1] == L'.' && sExt[2] == L'\0'))
		return;

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	IPtrPlisgoFSFile result = GetCustomTypeIconsFolder();

	PlisgoFSStorageFolder* pFolder = static_cast<PlisgoFSStorageFolder*>(result->GetAsFolder());

	if (pFolder->GetChild(sExt).get() != NULL)
		return;

	const std::string sData = (boost::format("%1% : %2%") %nList %nIndex).str();

	pFolder->AddChild(sExt, IPtrPlisgoFSFile(new PlisgoFSStringReadOnly(sData)));
}


void				RootPlisgoFSFolder::DisableStandardColumn(StdColumn eStdColumn)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	m_DisabledStandardColumn[(int)eStdColumn] = true;

	IPtrPlisgoFSFile file = GetChild(L".disable_std_columns");

	if (file.get() == NULL)
	{
		file.reset(new PlisgoFSStringFile);
		AddChild(L".disable_std_columns", file);
	}


	std::string sData;

	for(UINT n = 1; n < 7 ; ++n)
		if (m_DisabledStandardColumn[n])
		{
			sData += '0'+n;
			sData += ',';
		}

	PlisgoFSStringFile* pStrFile = dynamic_cast<PlisgoFSStringFile*>(file.get());

	if (pStrFile != NULL)
		pStrFile->SetString(sData);
}