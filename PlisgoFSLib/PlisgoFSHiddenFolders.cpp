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

	virtual bool				GetEntryFile( IPtrPlisgoFSFile&	rFile, const std::wstring&	rsRealFile) const = 0;

	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sNameUnknownCase) const
	{
		std::wstring sRealFile		= m_sSubjectPath;

		sRealFile+= sNameUnknownCase;

		IPtrPlisgoFSFile result;

		GetEntryFile(result,  sRealFile);

		return result;
	}


	virtual bool				ForEachChild(EachChild& rEachChild) const
	{
		IShellInfoFetcher::BasicFolder shelledFiles;

		if (!m_pRoot->GetShellInfoFetcher()->ReadShelled(m_sSubjectPath, &shelledFiles) || shelledFiles.size() == 0)
			return true;

		for(IShellInfoFetcher::BasicFolder::const_iterator it = shelledFiles.begin();
			it != shelledFiles.end(); ++it)
		{
			std::wstring sName = it->sName;

			IPtrPlisgoFSFile file = InternalGetChild(sName);

			if (file.get() != NULL && !rEachChild.Do(sName.c_str(), file))
				return false;
		}

		return true;
	}

protected:

	virtual IPtrPlisgoFSFile	InternalGetChild(std::wstring& rsName) const
	{
		return GetChild(rsName.c_str());
	}

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

	virtual bool				GetEntryFile(	IPtrPlisgoFSFile&	rFile,
												const std::wstring&	rsRealFile) const
	{
		std::wstring sText;

		if (!m_pRoot->GetShellInfoFetcher()->GetColumnEntry(rsRealFile, m_nColumnIndex, sText))
			return false;

		PlisgoFSStringFile* pStrFile = new PlisgoFSStringFile(sText, true);

		assert(pStrFile != NULL);

		pStrFile->SetVolatile(true);

		rFile.reset(pStrFile);

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


	virtual bool				GetEntryFile(	IPtrPlisgoFSFile&	rFile,
												const std::wstring&	rsRealFile) const
	{
		IconLocation icon;

		if (!m_pRoot->GetShellInfoFetcher()->GetOverlayIcon(rsRealFile, icon))
			return false;

		std::string sText;

		if (!icon.GetText(sText))
			return false;

		PlisgoFSStringFile* pStrFile = new PlisgoFSStringFile(sText, true);

		assert(pStrFile != NULL);

		pStrFile->SetVolatile(true);

		rFile.reset(pStrFile);

		return true;
	}
};


class CustomIconsFolder : public StubShellInfoFolder
{
public:
	CustomIconsFolder(	const std::wstring& rsSubjectPath,
						RootPlisgoFSFolder* pRoot)	: StubShellInfoFolder(rsSubjectPath, pRoot) {}

	virtual bool				GetEntryFile(	IPtrPlisgoFSFile&	rFile,
												const std::wstring&	rsRealFile) const
	{
		IconLocation icon;

		if (!m_pRoot->GetShellInfoFetcher()->GetCustomIcon(rsRealFile, icon))
			return false;

		std::string sText;

		if (!icon.GetText(sText))
			return false;

		PlisgoFSStringFile* pStrFile = new PlisgoFSStringFile(sText, true);

		assert(pStrFile != NULL);

		pStrFile->SetVolatile(true);

		rFile.reset(pStrFile);

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

	virtual bool				GetEntryFile(	IPtrPlisgoFSFile&	rFile,
												const std::wstring&	rsRealFile) const
	{
		const size_t nDot = rsRealFile.rfind(L'.');

		if (nDot == -1)
			return false;

		const std::wstring sRealFile = rsRealFile.substr(0,nDot);

		LPCWSTR sExt = &rsRealFile.c_str()[nDot];		

		if (!m_pRoot->GetShellInfoFetcher()->GetThumbnail(sRealFile, sExt, rFile))
			return false;

		rFile.reset(new VolatileEncapsulatedFile(rFile));

		return true;
	}

protected:

	virtual IPtrPlisgoFSFile	InternalGetChild(std::wstring& rsName) const
	{
		WCHAR* ExtTypes[3] = {L".jpg", L".bmp", L".png"};

		for(int n = 0; n < 3; ++n)
		{
			IPtrPlisgoFSFile result = GetChild((rsName + ExtTypes[n]).c_str());

			if (result.get() != NULL)
			{
				rsName += ExtTypes[n];

				return result;
			}
		}

		return IPtrPlisgoFSFile();
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

	virtual bool				GetEntryFile( IPtrPlisgoFSFile&, const std::wstring& ) const { return false; } //Not required

	virtual bool				ForEachChild(EachChild& rEachChild) const
	{
		InitVirtualFolder();

		//This shouldn't be called unless the user has for some crazy reason browsed in!

		IShellInfoFetcher::BasicFolder shelledFiles;

		if (!m_pRoot->GetShellInfoFetcher()->ReadShelled(m_sSubjectPath, &shelledFiles) || shelledFiles.size() == 0)
			return true;

		if (!m_virtualChildren.ForEachFile(rEachChild))
			return false;

		for(IShellInfoFetcher::BasicFolder::const_iterator it = shelledFiles.begin();
			it != shelledFiles.end(); ++it)
		{
			if (it->bIsFolder && !rEachChild.Do(it->sName, IPtrPlisgoFSFile(CreateChildShellInfoFolder(it->sName))))
				return false;
		}
		
		return true;
	}


	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sNameUnknownCase) const
	{
		InitVirtualFolder();

		assert(sNameUnknownCase != NULL);

		//This shouldn't be called unless the user has browsed in

		IShellInfoFetcher::BasicFolder shelledFiles;

		if (!m_pRoot->GetShellInfoFetcher()->ReadShelled(m_sSubjectPath, &shelledFiles))
			return IPtrPlisgoFSFile();

		IPtrPlisgoFSFile result = m_virtualChildren.GetFile(sNameUnknownCase);

		if (result.get() != NULL)
			return result;

		FileNameBuffer sName;

		CopyToLower(sName, sNameUnknownCase);

		for(IShellInfoFetcher::BasicFolder::const_iterator it = shelledFiles.begin();
			it != shelledFiles.end(); ++it)
		{
			if (it->bIsFolder && MatchLower(it->sName, sName))
				return CreateChildShellInfoFolder(it->sName);
		}

		return IPtrPlisgoFSFile();
	}


protected:

	void						InitVirtualFolder() const
	{
		//The whole const thing is because of cacheing, it's trying to avoid de-consting other than when init needs doing

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





static int GetSpecialFolderType(LPCWSTR sName)
{
	LPCWSTR sPos = sName;	

	if (ParseLower(sPos, L".overlay_icons") && (sPos[0] == L'\\' || sPos[0] == L'\0'))
		return 1;

	sPos = sName;
	if (ParseLower(sPos, L".custom_icons") && (sPos[0] == L'\\' || sPos[0] == L'\0'))
		return 2;

	sPos = sName;
	if (ParseLower(sPos, L".thumbnails") && (sPos[0] == L'\\' || sPos[0] == L'\0'))
		return 3;
	
	sPos = sName;
	if (ParseLower(sPos, L".column_"))
	{
		if (!isdigit(*sPos))
			return -1;

		++sPos;

		for(;sPos[0] != L'\\' && sPos[0] != L'\0';++sPos)
			if (!isdigit(*sPos))
				return -1;

		return 4;
	}

	return -1;
}


/*
***************************************************************
				RootPlisgoFSFolder
***************************************************************
*/


RootPlisgoFSFolder::RootPlisgoFSFolder(LPCWSTR sFSName, IShellInfoFetcher* pIShellInfoFetcher )
{
	assert(sFSName != NULL);

	boost::format fmt = boost::format("%1%") %PLISGO_APIVERSION;

	m_pIShellInfoFetcher = pIShellInfoFetcher;

	AddChild(L".fsname", IPtrPlisgoFSFile(new PlisgoFSStringFile( sFSName, true)));
	AddChild(L".version", IPtrPlisgoFSFile(new PlisgoFSStringFile(fmt.str(), true)));

	m_nColumnNum	= 0;
	m_nIconListsNum = 0;
	m_nRootMenuNum	= 0;

	m_bEnableThumbnails		= false;
	m_bEnableCustomIcons	= false;
	m_bEnableOverlays		= false;
	m_bHasCustomDefaultIcon	= false;
	m_bHasCustomFolderIcons = false;
	
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
												IPtrStringEvent	onClickEvent,
												int				nParentMenu,
												IPtrStringEvent enabledEvent,
												int				nIconList,
												int				nIconIndex)
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

		pMenuFolder->AddChild(sMenuName.c_str(), IPtrPlisgoFSFile(new PlisgoFSMenuItem(onClickEvent, enabledEvent, sText, nIconList, nIconIndex)));
	}
	else
	{
		std::wstring sMenuName = (boost::wformat(L".menu_%1%") %m_nRootMenuNum).str();

		nResult = (int)m_nRootMenuNum;

		++m_nRootMenuNum;

		AddChild(sMenuName.c_str(), IPtrPlisgoFSFile(new PlisgoFSMenuItem(onClickEvent, enabledEvent, sText, nIconList, nIconIndex)));
	}

	return nResult;
}


bool				RootPlisgoFSFolder::AddColumn(std::wstring sHeader)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (m_pIShellInfoFetcher == NULL)
		return false;

	boost::wformat headerFmt = boost::wformat(L".column_header_%1%") %m_nColumnNum;

	AddChild(headerFmt.str().c_str(), IPtrPlisgoFSFile(new PlisgoFSStringFile(sHeader, true)));

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

	AddChild(sName.c_str(), IPtrPlisgoFSFile(new PlisgoFSStringFile((eAlignment == LEFT)?L"l":L"r", true)));

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

	AddChild(sName.c_str(), IPtrPlisgoFSFile(new PlisgoFSStringFile((eType == INT)?L"i":L"f", true)));

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

	AddChild(sName.c_str(), IPtrPlisgoFSFile(new PlisgoFSStringFile(sBuffer, true)));

	return true;
}


bool				RootPlisgoFSFolder::EnableThumbnails()
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (m_pIShellInfoFetcher == NULL)
		return false;

	m_bEnableThumbnails = true;
	
	EnableCustomShell();

	return true;
}


bool				RootPlisgoFSFolder::EnableCustomIcons()
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (m_pIShellInfoFetcher == NULL)
		return false;

	m_bEnableCustomIcons = true;
	
	EnableCustomShell();

	return true;
}


bool				RootPlisgoFSFolder::EnableOverlays()
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (m_pIShellInfoFetcher == NULL)
		return false;

	m_bEnableOverlays = true;

	EnableCustomShell();
	
	return true;
}


void				RootPlisgoFSFolder::EnableCustomShell()
{
	if (GetChild(L".shellinfo").get() == NULL)
		AddChild(L".shellinfo", IPtrPlisgoFSFile(new ShellInfoFolder(L"\\", this)));
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

	pFolder->AddChild(sExt, IPtrPlisgoFSFile(new PlisgoFSStringFile(sData, true)));
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