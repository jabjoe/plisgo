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

class StubShellInfoFolder : public PlisgoFSStdPathFile<PlisgoFSFolder>
{
public:
	StubShellInfoFolder(const std::wstring& rsPath,
						const std::wstring& rsSubjectPath,
						RootPlisgoFSFolder* pRoot)	: PlisgoFSStdPathFile<PlisgoFSFolder>(rsPath)
	{
		assert(pRoot != NULL);

		m_sSubjectPath	= rsSubjectPath;

		if (m_sSubjectPath.length() == 0)
			m_sSubjectPath = L"\\";
		else if (m_sSubjectPath[m_sSubjectPath.length()-1] != L'\\')
			m_sSubjectPath += L"\\";

		m_pRoot			= pRoot;
	}

	virtual IPtrPlisgoFSFile	GetDescendant(LPCWSTR sPath) const	{ return GetChild(sPath); } //The tree ends here

	virtual bool				GetEntryFile(	IPtrPlisgoFSFile&	rFile,
												const std::wstring& rsVirtualFile,
												const std::wstring&	rsRealFile) const = 0;

	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sNameUnknownCase) const
	{
		std::wstring sVirtualFile	= m_sPath + L"\\" += sNameUnknownCase;
		std::wstring sRealFile		= m_sSubjectPath;

		sRealFile+= sNameUnknownCase;

		IPtrPlisgoFSFile result;

		GetEntryFile(result, sVirtualFile, sRealFile);

		return result;
	}


	virtual bool				ForEachChild(EachChild& rEachChild) const
	{
		IShellInfoFetcher::BasicFolder shelledFolders;

		if (!m_pRoot->GetShellInfoFetcher()->ReadShelled(m_sSubjectPath, shelledFolders) || shelledFolders.size() == 0)
			return true;

		for(IShellInfoFetcher::BasicFolder::const_iterator it = shelledFolders.begin();
			it != shelledFolders.end(); ++it)
		{
			IPtrPlisgoFSFile file = InternalGetChild(it->sName.c_str());

			if (file.get() != NULL && !rEachChild.Do(file))
				return false;
		}

		return true;
	}


	virtual UINT				GetChildNum() const
	{
		IShellInfoFetcher::BasicFolder shelledFolders;

		if (!m_pRoot->GetShellInfoFetcher()->ReadShelled(m_sSubjectPath, shelledFolders) || shelledFolders.size() == 0)
			return 0;

		int nResult = 0;

		for(IShellInfoFetcher::BasicFolder::const_iterator it = shelledFolders.begin();
			it != shelledFolders.end(); ++it)
		{
			IPtrPlisgoFSFile file = InternalGetChild(it->sName.c_str());

			if (file.get() != NULL)
				++nResult;
		}

		return nResult;
	}

protected:

	virtual IPtrPlisgoFSFile	InternalGetChild(const std::wstring& rsName) const
	{
		return GetChild(rsName.c_str());
	}

	std::wstring				m_sSubjectPath;
	RootPlisgoFSFolder*			m_pRoot;
};





class ColumnFolder : public StubShellInfoFolder
{
public:

	ColumnFolder(	const std::wstring& rsPath,
					const std::wstring& rsSubjectPath,
					RootPlisgoFSFolder* pRoot,
					const int nColumn) : StubShellInfoFolder(rsPath, rsSubjectPath, pRoot)
	{
		m_nColumnIndex = nColumn;
	}

	virtual bool				GetEntryFile(	IPtrPlisgoFSFile&	rFile,
												const std::wstring& rsVirtualFile,
												const std::wstring&	rsRealFile) const
	{
		std::wstring sText;

		if (!m_pRoot->GetShellInfoFetcher()->GetColumnEntry(rsRealFile, m_nColumnIndex, sText))
			return false;

		rFile = IPtrPlisgoFSFile( new PlisgoFSStringFile(rsVirtualFile, sText, true));	

		return true;
	}

private:
	 int m_nColumnIndex;
};


class OverlayIconsFolder : public StubShellInfoFolder
{
public:
	OverlayIconsFolder(	const std::wstring& rsPath,
						const std::wstring& rsSubjectPath,
						RootPlisgoFSFolder* pRoot)	: StubShellInfoFolder(rsPath, rsSubjectPath, pRoot) {}


	virtual bool				GetEntryFile(	IPtrPlisgoFSFile&	rFile,
												const std::wstring& rsVirtualFile,
												const std::wstring&	rsRealFile) const
	{
		IconLocation icon;

		if (!m_pRoot->GetShellInfoFetcher()->GetOverlayIcon(rsRealFile, icon))
			return false;

		std::string sText;

		if (!icon.GetText(sText))
			return false;

		rFile = IPtrPlisgoFSFile( new PlisgoFSStringFile(rsVirtualFile, sText, true));	

		return true;
	}
};


class CustomIconsFolder : public StubShellInfoFolder
{
public:
	CustomIconsFolder(	const std::wstring& rsPath,
						const std::wstring& rsSubjectPath,
						RootPlisgoFSFolder* pRoot)	: StubShellInfoFolder(rsPath, rsSubjectPath, pRoot) {}

	virtual bool				GetEntryFile(	IPtrPlisgoFSFile&	rFile,
												const std::wstring& rsVirtualFile,
												const std::wstring&	rsRealFile) const
	{
		IconLocation icon;

		if (!m_pRoot->GetShellInfoFetcher()->GetCustomIcon(rsRealFile, icon))
			return false;

		std::string sText;

		if (!icon.GetText(sText))
			return false;

		rFile = IPtrPlisgoFSFile( new PlisgoFSStringFile(rsVirtualFile, sText, true));	

		return true;
	}
};


class ThumbnailsFolder : public StubShellInfoFolder
{
public:

	ThumbnailsFolder(	const std::wstring& rsPath,
						const std::wstring& rsSubjectPath,
						RootPlisgoFSFolder* pRoot)	: StubShellInfoFolder(rsPath, rsSubjectPath, pRoot) {}

	virtual bool				GetEntryFile(	IPtrPlisgoFSFile&	rFile,
												const std::wstring& rsVirtualFile,
												const std::wstring&	rsRealFile) const
	{

		const size_t nDot = rsRealFile.rfind(L'.');

		const std::wstring sRealFile = (nDot == -1)?rsRealFile:rsRealFile.substr(0,nDot);

		return m_pRoot->GetShellInfoFetcher()->GetThumbnail(sRealFile, rsVirtualFile, rFile);
	}

protected:

	virtual IPtrPlisgoFSFile	InternalGetChild(const std::wstring& rsName) const
	{
		WCHAR* ExtTypes[3] = {L".jpg", L".bmp", L".png"};

		for(int n = 0; n < 3; ++n)
		{
			IPtrPlisgoFSFile result = GetChild((rsName + ExtTypes[n]).c_str());

			if (result.get() != NULL)
				return result;
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

	ShellInfoFolder(	const std::wstring& rsPath,
						const std::wstring& rsSubjectPath,
						RootPlisgoFSFolder* pRoot) : StubShellInfoFolder(rsPath, rsSubjectPath, pRoot)
	{}

	virtual IPtrPlisgoFSFile	GetDescendant(LPCWSTR sPath) const;

	virtual bool				GetEntryFile( IPtrPlisgoFSFile&, const std::wstring&, const std::wstring& ) const { return false; } //Not required

	virtual bool				ForEachChild(EachChild& rEachChild) const
	{
		//This shouldn't be called unless the user has for some crazy reason browsed in!

		IShellInfoFetcher::BasicFolder shelledFolders;

		if (!m_pRoot->GetShellInfoFetcher()->ReadShelled(m_sSubjectPath, shelledFolders) || shelledFolders.size() == 0)
			return true;

		if (m_pRoot->HasOverlays() && !rEachChild.Do(IPtrPlisgoFSFile(new OverlayIconsFolder(m_sPath + L"\\.overlay_icons", m_sSubjectPath, m_pRoot))))
			return false;

		if (m_pRoot->HasCustomIcons() && !rEachChild.Do(IPtrPlisgoFSFile(new CustomIconsFolder(m_sPath + L"\\.custom_icons", m_sSubjectPath, m_pRoot))))
			return false;

		if (m_pRoot->HasThumbnails() && !rEachChild.Do(IPtrPlisgoFSFile(new ThumbnailsFolder(m_sPath + L"\\.thumbnails", m_sSubjectPath, m_pRoot))))
			return false;

		const UINT nColumnNum = m_pRoot->GetColumnNum();

		for(UINT n = 0; n < nColumnNum; ++n)
			if (!rEachChild.Do(IPtrPlisgoFSFile(CreateColumnFolder((int)n))))
				return false;

		for(IShellInfoFetcher::BasicFolder::const_iterator it = shelledFolders.begin();
			it != shelledFolders.end(); ++it)
		{
			if (it->bIsFolder && !rEachChild.Do(IPtrPlisgoFSFile(CreateChildShellInfoFolder(it->sName))))
				return false;
		}
		
		return true;
	}


	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sNameUnknownCase) const
	{
		assert(sNameUnknownCase != NULL);

		//This shouldn't be called unless the user has browsed in

		IShellInfoFetcher::BasicFolder shelledFolders;

		if (!m_pRoot->GetShellInfoFetcher()->ReadShelled(m_sSubjectPath, shelledFolders) || shelledFolders.size() == 0)
			return IPtrPlisgoFSFile();

		WCHAR sName[MAX_PATH];

		CopyToLower(sName, MAX_PATH, sNameUnknownCase);

		StubShellInfoFolder* pFolder = NULL;

		if (!GetStubShellInfoFolder(pFolder, sName, m_sSubjectPath, (m_sPath + L"\\") += sName))
			return IPtrPlisgoFSFile();

		if (pFolder != NULL)
			return IPtrPlisgoFSFile(pFolder);

		for(IShellInfoFetcher::BasicFolder::const_iterator it = shelledFolders.begin();
			it != shelledFolders.end(); ++it)
		{
			if (it->bIsFolder && MatchLower(it->sName.c_str(), sName))
				return CreateChildShellInfoFolder(it->sName);
		}

		return IPtrPlisgoFSFile();
	}


	virtual UINT				GetChildNum() const
	{
		//This shouldn't be called unless the user has browsed in

		UINT nResult = m_pRoot->GetColumnNum();
		
		if (m_pRoot->HasOverlays())
			++nResult;

		if (m_pRoot->HasCustomIcons())
			++nResult;

		if (m_pRoot->HasThumbnails())
			++nResult;

		IShellInfoFetcher::BasicFolder shelledFolders;

		if (!m_pRoot->GetShellInfoFetcher()->ReadShelled(m_sSubjectPath, shelledFolders))
			return 0;

		for(IShellInfoFetcher::BasicFolder::const_iterator it = shelledFolders.begin();
			it != shelledFolders.end(); ++it)
		{
			if (it->bIsFolder)
				++nResult;
		}

		return nResult;
	}


protected:
	IPtrPlisgoFSFile			CreateChildShellInfoFolder(const std::wstring& rsName) const
	{
		return IPtrPlisgoFSFile( new ShellInfoFolder(	m_sPath + L"\\" + rsName,
				(m_sSubjectPath.length() == 1)?(m_sSubjectPath + rsName):(m_sSubjectPath + L"\\" + rsName),
														m_pRoot));
	}

	bool						GetStubShellInfoFolder(	StubShellInfoFolder*& rpFolder,
														LPCWSTR sName,
														const std::wstring& rsRealPath,
														const std::wstring& rsVirtualPath) const;

	IPtrPlisgoFSFile			CreateColumnFolder(const int nColumn) const;
};



IPtrPlisgoFSFile	ShellInfoFolder::CreateColumnFolder(const int nColumn) const
{
	WCHAR sName[MAX_PATH];

	wsprintfW(sName, L"\\.column_%i", nColumn);

	return IPtrPlisgoFSFile( new ColumnFolder(m_sPath + sName, m_sSubjectPath, m_pRoot, nColumn));
}


bool				ShellInfoFolder::GetStubShellInfoFolder(StubShellInfoFolder*&	rpFolder,
															LPCWSTR					sName,
															const std::wstring&		rsRealPath,
															const std::wstring&		rsVirtualPath) const
{
	if (m_pRoot->HasOverlays())
	{
		if (wcscmp(sName, L".overlay_icons") == 0)
		{
			if (m_pRoot->GetShellInfoFetcher()->IsShelledFolder(rsRealPath))
			{
				rpFolder = new OverlayIconsFolder(rsVirtualPath, rsRealPath, m_pRoot);
				return true;
			}
			else return false;
		}
	}

	if (m_pRoot->HasCustomIcons())
	{
		if (wcscmp(sName, L".custom_icons") == 0)
		{
			if (m_pRoot->GetShellInfoFetcher()->IsShelledFolder(rsRealPath))
			{
				rpFolder = new CustomIconsFolder(rsVirtualPath, rsRealPath, m_pRoot);
				return true;
			}
			else return false;
		}
	}

	if (m_pRoot->HasThumbnails())
	{
		if (wcscmp(sName, L".thumbnails") == 0)
		{
			if (m_pRoot->GetShellInfoFetcher()->IsShelledFolder(rsRealPath))
			{
				rpFolder = new ThumbnailsFolder(rsVirtualPath, rsRealPath, m_pRoot);
				return true;
			}
			else return false;
		}
	}

	const int nColumnNum = (int)m_pRoot->GetColumnNum();

	if (nColumnNum)
	{
		if (!ParseLower(sName, L".column_"))
			return true;
		
		const int nColumn = _wtoi(sName);

		if (nColumn < 0 || nColumn >= nColumnNum)
			return false;

		if (!m_pRoot->GetShellInfoFetcher()->IsShelledFolder(rsRealPath))
			return false;

		rpFolder = new ColumnFolder(rsVirtualPath, rsRealPath, m_pRoot, nColumn);
	}
	
	return true;
}






IPtrPlisgoFSFile	ShellInfoFolder::GetDescendant(LPCWSTR sPath) const
{
	assert(sPath != NULL);

	LPCWSTR sNameUnknownCase = GetNameFromPath(sPath);

	std::wstring sRealPath = L"\\";
	
	sRealPath.append(sPath, (sNameUnknownCase != sPath)?((sNameUnknownCase-sPath)-1):0);
	
	if (m_pRoot->GetShellInfoFetcher()->IsShelledFolder(sRealPath))
	{
		std::wstring sVirtualPath = L"\\.plisgofs\\.shellinfo\\";
		sVirtualPath += sPath;

		if (sVirtualPath.length() && sVirtualPath[sVirtualPath.length()-1] == L'\\')
			sVirtualPath.resize(sVirtualPath.size()-1);

		//Check if it's retrieving a StubShellInfoFolder folder
		{
			WCHAR sName[MAX_PATH];

			CopyToLower(sName, MAX_PATH, sNameUnknownCase);

			StubShellInfoFolder* pFolder = NULL;

			LPWSTR sTrailing = wcschr(sName,L'\\');

			if (sTrailing != NULL)
				sTrailing[0] = L'\0';

			if (!GetStubShellInfoFolder(pFolder, sName, sRealPath, sVirtualPath))
				return IPtrPlisgoFSFile();

			if (pFolder != NULL)
				return IPtrPlisgoFSFile(pFolder);
		}


		std::wstring sFullRealPath = sRealPath;

		if (sFullRealPath.length() && sFullRealPath[sFullRealPath.length()-1] != L'\\')
			sFullRealPath += L"\\";

		sFullRealPath+= sNameUnknownCase;

		if (!m_pRoot->GetShellInfoFetcher()->IsShelledFolder(sFullRealPath))
			return IPtrPlisgoFSFile();


		return IPtrPlisgoFSFile( new ShellInfoFolder( sVirtualPath, sFullRealPath, m_pRoot));
	}
	else
	{
		//Check if it's retrieving a child of a StubShellInfoFolder folder

		LPCWSTR sParentNameUnknownCase = sNameUnknownCase-2; //Skip past slash

		while(sParentNameUnknownCase > sPath && sParentNameUnknownCase[0] != L'\\')
			--sParentNameUnknownCase;

		if (sParentNameUnknownCase[0] == L'\\')
			++sParentNameUnknownCase;

		WCHAR sName[MAX_PATH];
		
		int n = 0;

		for(; sParentNameUnknownCase[n] != L'\\' && ((sParentNameUnknownCase+n) < sNameUnknownCase); ++n)
			sName[n] = tolower(sParentNameUnknownCase[n]);

		sName[n] = L'\0';

		sRealPath = L"\\";
		sRealPath.append(sPath, (sParentNameUnknownCase != sPath)?(sParentNameUnknownCase-sPath):0);

		std::wstring sVirtualPath = L"\\.plisgofs\\.shellinfo\\";
		
		sVirtualPath.append(sPath, (sNameUnknownCase-sPath)-1);

		StubShellInfoFolder* pFolder = NULL;

		if (!GetStubShellInfoFolder(pFolder, sName, sRealPath, sVirtualPath))
			return IPtrPlisgoFSFile();

		if (pFolder != NULL)
		{
			IPtrPlisgoFSFile result = pFolder->GetChild(sNameUnknownCase);

			delete pFolder;

			return result;
		}
	}

	return IPtrPlisgoFSFile();
}


/*
***************************************************************
				RootPlisgoFSFolder
***************************************************************
*/


RootPlisgoFSFolder::RootPlisgoFSFolder(LPCWSTR sFSName, IShellInfoFetcher* pIShellInfoFetcher ) : PlisgoFSStorageFolder(L"\\.plisgofs")
{
	assert(sFSName != NULL);

	boost::format fmt = boost::format("%1%") %PLISGO_APIVERSION;

	m_pIShellInfoFetcher = pIShellInfoFetcher;

	AddChild(IPtrPlisgoFSFile(new PlisgoFSStringFile(L"\\.plisgofs\\.fsname", sFSName, true)));
	AddChild(IPtrPlisgoFSFile(new PlisgoFSStringFile(L"\\.plisgofs\\.version", fmt.str(), true)));

	m_nColumnNum	= 0;
	m_nIconListsNum = 0;
	m_nRootMenuNum	= 0;

	m_bEnableThumbnails		= false;
	m_bEnableCustomIcons	= false;
	m_bEnableOverlays		= false;
	m_bHasCustomDefaultIcon	= false;
	m_bHasCustomFolderIcons = false;
}


	
UINT				RootPlisgoFSFolder::GetColumnNum() const			{ boost::shared_lock<boost::shared_mutex> lock(m_Mutex); return m_nColumnNum; }
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
					Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);

					if (pBitmap != NULL)
					{
						UINT nHeight = pBitmap->GetHeight();

						delete pBitmap;

						boost::wformat nameFmt = boost::wformat(L".icons_%1%_%2%.%3%") %nListIndex %nHeight %sExt;

						if (GetChild(nameFmt.str().c_str()).get() == NULL)
						{
							std::wstring sPath = std::wstring(L"\\.plisgofs\\") + nameFmt.str();

							AddChild(IPtrPlisgoFSFile(new PlisgoFSDataFile(sPath, (BYTE*)pData, nSize )));

							if (nListIndex >= m_nIconListsNum)
								m_nIconListsNum = nListIndex+1;

							bResult = true;
						}
					}

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

	Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromFile(sFilename.c_str(), TRUE);

	if (pBitmap != NULL)
	{
		UINT nHeight = pBitmap->GetHeight();

		delete pBitmap;

		size_t nExtPos = sFilename.rfind(L".");

		boost::wformat nameFmt = boost::wformat(L".icons_%1%_%2%%3%") %nListIndex %nHeight %sFilename.substr(nExtPos);

		if (GetChild(nameFmt.str().c_str()).get() == NULL)
		{
			std::wstring sPath = std::wstring(L"\\.plisgofs\\") + nameFmt.str();

			AddChild(IPtrPlisgoFSFile(new PlisgoFSRedirectionFile(sPath, sFilename)));

			if (nListIndex >= m_nIconListsNum)
				m_nIconListsNum = nListIndex+1;

			return true;
		}
	}

	return false;
}



class FindMenuCallObj : public PlisgoFSFolder::EachChild
{
public:
	FindMenuCallObj(int& rnMenu) : m_rnMenu(rnMenu) {}
	
	void operator = (FindMenuCallObj&)	{}

	virtual bool Do(IPtrPlisgoFSFile file)
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
	
	virtual bool Do(IPtrPlisgoFSFile file)
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

	assert(nIconList < m_nIconListsNum);

	IPtrPlisgoFSFile parentMenu = FindMenu(nParentMenu);

	int nResult = -1;

	if (parentMenu.get() != NULL)
	{		
		PlisgoFSMenuItem* pMenuFolder = static_cast<PlisgoFSMenuItem*>(parentMenu.get());

		FolderCountCallObj	counter;

		pMenuFolder->ForEachChild(counter);

		nResult = counter.m_nCount;

		std::wstring sMenuPath = (boost::wformat(L"%1%\\.menu_%2%") %pMenuFolder->GetPath() %nResult).str();

		pMenuFolder->AddChild(IPtrPlisgoFSFile(new PlisgoFSMenuItem(sMenuPath, onClickEvent, enabledEvent, sText, nIconList, nIconIndex)));
	}
	else
	{
		std::wstring  sMenuPath = (boost::wformat(L"\\.plisgofs\\.menu_%1%") %m_nRootMenuNum).str();

		nResult = (int)m_nRootMenuNum;

		++m_nRootMenuNum;

		AddChild(IPtrPlisgoFSFile(new PlisgoFSMenuItem(sMenuPath, onClickEvent, enabledEvent, sText, nIconList, nIconIndex)));
	}

	return nResult;
}


bool				RootPlisgoFSFolder::AddColumn(std::wstring sHeader)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (m_pIShellInfoFetcher == NULL)
		return false;

	boost::wformat headerFmt = boost::wformat(L"\\.plisgofs\\.column_header_%1%") %m_nColumnNum;

	AddChild(IPtrPlisgoFSFile(new PlisgoFSStringFile(headerFmt.str(), sHeader, true)));

	++m_nColumnNum;

	EnableCustomShell();

	return true;
}


bool				RootPlisgoFSFolder::SetColumnAlignment(UINT nColumnIndex, ColumnAlignment eAlignment)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (nColumnIndex >= m_nColumnNum)
		return false;

	std::wstring sPath = (boost::wformat(L".column_alignment_%1%") %nColumnIndex).str();

	if (GetChild(sPath.c_str()).get() != NULL)
		return false;

	if (eAlignment == CENTER)
		return true;

	sPath.insert(0, L"\\.plisgofs\\", 11);

	AddChild(IPtrPlisgoFSFile(new PlisgoFSStringFile(sPath, (eAlignment == LEFT)?L"l":L"r", true)));

	return true;
}


bool				RootPlisgoFSFolder::SetColumnType(UINT nColumnIndex, ColumnType eType)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (nColumnIndex >= m_nColumnNum)
		return false;

	std::wstring sPath = (boost::wformat(L".column_type_%1%") %nColumnIndex).str();

	if (GetChild(sPath.c_str()).get() != NULL)
		return false;

	if (eType == TEXT)
		return true;

	sPath.insert(0, L"\\.plisgofs\\", 11);

	AddChild(IPtrPlisgoFSFile(new PlisgoFSStringFile(sPath, (eType == INT)?L"i":L"f", true)));

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
		AddChild(IPtrPlisgoFSFile(new ShellInfoFolder(L"\\.plisgofs\\.shellinfo", L"\\", this)));
}


IPtrPlisgoFSFile	RootPlisgoFSFolder::GetCustomTypeIconsFolder()
{
	IPtrPlisgoFSFile result = GetChild(L".type_icons");

	if (result == NULL)
	{
		result.reset(new PlisgoFSStorageFolder(L"\\.plisgofs\\.type_icons"));

		assert(result.get() != NULL);

		AddChild(result);
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

	pFolder->AddChild(IPtrPlisgoFSFile(new PlisgoFSStringFile(std::wstring(L"\\.plisgofs\\.type_icons\\") + sExt, sData, true)));
}