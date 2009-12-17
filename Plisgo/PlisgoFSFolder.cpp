/*
    Copyright 2009 Eurocom Entertainment Software Limited

    This file is part of Plisgo.

    Eurocom Entertainment Software Limited, hereby disclaims all
    copyright interest in “Plisgo” written by Joe Burmeister.

    Plisgo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Plisgo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Plisgo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include "PlisgoFSFolder.h"
#include "PlisgoFSFolderReg.h"



static size_t FindFileNameEntry(LPCWSTR sPath, const std::wstring& rsBuffer)
{
	assert(sPath != NULL);

	LPCWSTR sName = wcsrchr(sPath, L'\\');

	if (sName != NULL)
		++sName;
	else
		sName = sPath;

	size_t nNameLen = wcslen(sName);

	if (nNameLen+1 > MAX_PATH)
		return false;

	WCHAR sName2[MAX_PATH] = L"\r";

	FillLowerCopy(sName2, MAX_PATH, sName);

	size_t nIndex = rsBuffer.find(sName2);

	if (nIndex == 0)
	{
		if (rsBuffer[nIndex+nNameLen] == L'\t')
			return nIndex+nNameLen;

		nIndex = rsBuffer.find(sName2, nIndex+1);
	}

	while(nIndex != -1)
	{
		if ((rsBuffer[nIndex-1] == L'\n' || rsBuffer[nIndex-1] == L'\r')
			&& rsBuffer[nIndex+nNameLen] == L'\t')
		{
			return nIndex+nNameLen;
		}

		nIndex = rsBuffer.find(sName2, nIndex+1);
	}

	return -1;

}


static bool ReadFileIndices(LPCWSTR sName, const std::wstring& rsBuffer, UINT& rnList, UINT& rnIndex)
{
	const size_t nEntryIndex = FindFileNameEntry(sName, rsBuffer);

	if (nEntryIndex != -1)
	{
		const size_t nDevide = rsBuffer.find(L':', nEntryIndex+1);

		if (nDevide != -1)
		{
			rnList	= (UINT)_wtoi(&rsBuffer.c_str()[nEntryIndex+1]);
			rnIndex	= (UINT)_wtoi(&rsBuffer.c_str()[nDevide+1]);

			return true;
		}
	}

	return false;
}


const WCHAR*	GetThumbnailExt(const std::wstring&	rsThumbnail)
{
	if (GetFileAttributes((rsThumbnail + L".jpg").c_str()) != INVALID_FILE_ATTRIBUTES)
		return L".jpg";

	if (GetFileAttributes((rsThumbnail + L".bmp").c_str()) != INVALID_FILE_ATTRIBUTES)
		return L".bmp";

	if (GetFileAttributes((rsThumbnail + L".png").c_str()) != INVALID_FILE_ATTRIBUTES)
		return L".png";

	return NULL;
}


static ULONG64	GetLastWriteTime(LPCWSTR sFile)
{
	assert(sFile != NULL);

	WIN32_FILE_ATTRIBUTE_DATA info = {0};

	GetFileAttributesEx(sFile, GetFileExInfoStandard, &info);

	return *(ULONG64*)&info.ftLastWriteTime;
}

static int		NotAlphaNumeric(WCHAR c)	{ return !isalnum(c); }


PlisgoFSRoot::PlisgoFSRoot(const std::wstring& rsPath, IconRegistry* pIconRegistry)
{	
	m_sPath = rsPath;

	assert(m_sPath.size());

	if (*(m_sPath.end()-1) != L'\\')
		m_sPath += L'\\';

	std::wstring sPath = m_sPath + L".plisgofs\\";

	m_nFSVersion = -1;

	if (!ReadTextFromFile(m_sFSName, (sPath + L".fsname").c_str()) ||
		!ReadIntFromFile(m_nFSVersion, (sPath + L".version").c_str()) || m_nFSVersion != PLISGO_APIVERSION)
	{
		m_sFSName.resize(0);

		return;
	}

	//Ensure it could be used as a filename
	m_sFSName.erase(std::remove_if(m_sFSName.begin(), m_sFSName.end(), NotAlphaNumeric), m_sFSName.end());

	m_IconRegistry = pIconRegistry->GetFSIconRegistry(m_sFSName.c_str(), m_nFSVersion, sPath);

	assert(m_IconRegistry.get() != NULL);

	std::wstring sTemp(sPath + L".column_header_*");

	WIN32_FIND_DATA findData;

	HANDLE hFind = FindFirstFileW(sTemp.c_str(), &findData);

	if (hFind != NULL && hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			std::wstring sHeader;
			
			ReadTextFromFile(sHeader, (sPath + findData.cFileName).c_str());

			int nIndex = _wtoi(findData.cFileName + 15);//".column_header_"

			if (nIndex >= (int)m_Columns.size())
				m_Columns.resize(nIndex+1);

			ColumnDef& rColumnDef = m_Columns[nIndex];

			rColumnDef.sHeader = sHeader;

			std::wstring sTemp;

			WCHAR sBuffer[MAX_PATH];

			wsprintf(sBuffer, L".column_alignment_%i", nIndex);

			ReadTextFromFile(sTemp, (sPath + sBuffer).c_str());

			if (sTemp.length())
			{
				WCHAR nAlignment = tolower(sTemp[0]);

				if (nAlignment == L'l')
					rColumnDef.eAlignment = LEFT;
				else if (nAlignment == L'r')
					rColumnDef.eAlignment = RIGHT;
			}

			sTemp.clear();

			wsprintf(sBuffer, L".column_type_%i", nIndex);

			ReadTextFromFile(sTemp, (sPath + sBuffer).c_str());

			if (sTemp.length())
			{
				//it's not TEXT type or we wouldn't be here.

				if (rColumnDef.eAlignment == CENTER)
					rColumnDef.eAlignment = RIGHT;

				WCHAR nType = tolower(sTemp[0]);

				if (nType == L'i')
					rColumnDef.eType = INT;
				else if (nType == L'f')
					rColumnDef.eType = FLOAT;
			}

			sTemp.clear();

			wsprintf(sBuffer, L".column_width_%i", nIndex);

			ReadIntFromFile(rColumnDef.nWidth, (sPath + sBuffer).c_str());
		}
		while(FindNextFileW(hFind, &findData) != 0);

		FindClose(hFind);
	}
}


PlisgoFSRoot::~PlisgoFSRoot()
{
	std::wstring sPath = m_sPath + L".plisgofs\\";

	if (m_IconRegistry.get() != NULL)
		m_IconRegistry->GetMainIconRegistry()->ReleaseFSIconRegistry(m_IconRegistry, sPath);
}


void	PlisgoFSRoot::GetMenuItems(IPtrPlisgoFSMenuList& rMenus, const WStringList& rSelection)
{
	PlisgoFSMenu::LoadMenus(rMenus, m_IconRegistry, m_sPath + L".plisgofs\\", rSelection);
}


bool	PlisgoFSRoot::GetFolderIcon(IconLocation& rIconLocation, const UINT nHeight, bool bOpen ) const
{
	if (bOpen)
		return m_IconRegistry->ReadIconLocation(rIconLocation, (m_sPath + L".plisgofs\\.type_icons\\.folder_icon_open").c_str(), nHeight);
	else
		return m_IconRegistry->ReadIconLocation(rIconLocation, (m_sPath + L".plisgofs\\.type_icons\\.folder_icon_closed").c_str(), nHeight);
}


bool	PlisgoFSRoot::GetExtensionIcon(IconLocation& rIconLocation, LPCWSTR sExt, const UINT nHeight) const
{
	if (sExt != NULL)
		if (m_IconRegistry->ReadIconLocation(rIconLocation, (m_sPath + L".plisgofs\\.type_icons\\" + sExt).c_str(), nHeight))
			return true;
	
	return m_IconRegistry->ReadIconLocation(rIconLocation, (m_sPath + L".plisgofs\\.type_icons\\.default_file").c_str(), nHeight);

}


bool	PlisgoFSRoot::GetPathColumnEntryPath(std::wstring& rsOutPath, const std::wstring& rsInPath, int nIndex) const
{
	std::wstring sName;
	std::wstring sShellInfoFolder;

	if (!GetShellInfoFolder(sName, sShellInfoFolder, rsInPath))
		return false;

	rsOutPath = sShellInfoFolder;
	rsOutPath += (boost::wformat(L".column_%1%\\") %nIndex).str();
	rsOutPath += sName;

	return true;
}


bool	PlisgoFSRoot::GetPathColumnTextEntry(std::wstring& rsResult, const std::wstring& rsPath, int nIndex) const
{
	std::wstring sPath;

	if (!GetPathColumnEntryPath(sPath, rsPath, nIndex))
		return false;

	return ReadTextFromFile(rsResult, sPath.c_str());
}


bool	PlisgoFSRoot::GetPathColumnIntEntry(int& rnResult, const std::wstring& rsPath, int nIndex) const
{
	std::wstring sPath;

	if (!GetPathColumnEntryPath(sPath, rsPath, nIndex))
		return false;

	return ReadIntFromFile(rnResult, sPath.c_str());
}


bool	PlisgoFSRoot::GetPathColumnFloatEntry(double& rnResult, const std::wstring& rsPath, int nIndex) const
{
	std::wstring sPath;

	if (!GetPathColumnEntryPath(sPath, rsPath, nIndex))
		return false;

	return ReadDoubleFromFile(rnResult, sPath.c_str());
}


int		PlisgoFSRoot::GetPathIconIndex(const std::wstring& rsPath, IPtrRefIconList iconList, bool bOpen) const
{
	IconLocation Location;

	int nResult = DEFAULTFILEICONINDEX;

	if (GetPathIconLocation(Location, rsPath, iconList, bOpen))
		iconList->GetIconLocationIndex(nResult, Location);

	return nResult;
}



bool	PlisgoFSRoot::GetCustomIconIconLocation(IconLocation&		rIconLocation,
												const std::wstring& rsShellInfoFolder,
												const std::wstring& rsName,
												const UINT			nHeight) const
{
	return m_IconRegistry->ReadIconLocation(rIconLocation,
											rsShellInfoFolder + L".custom_icons\\" += rsName,
											nHeight);
}


bool	PlisgoFSRoot::GetThumbnailFile(	std::wstring&		rsFile,
										const std::wstring& rsShellInfoFolder,
										const std::wstring& rsName) const
{
	const std::wstring sThumbnail = rsShellInfoFolder + L".thumbnails\\" += rsName;

	const WCHAR* sExt = GetThumbnailExt(sThumbnail);

	if (sExt == NULL)		
		return false;

	rsFile = sThumbnail + sExt;

	return true;
}


bool	PlisgoFSRoot::GetThumbnailIconLocation(	IconLocation&		rIconLocation,
												const std::wstring& rsShellInfoFolder,
												const std::wstring& rsName,
												const UINT			nHeight) const
{
	if (!GetThumbnailFile(rIconLocation.sPath, rsShellInfoFolder, rsName))
		return false;

	if (GetFileAttributes(rIconLocation.sPath.c_str()) == INVALID_FILE_ATTRIBUTES)
		return false;

	rIconLocation.nIndex	= 0;

	return true;
}


bool	PlisgoFSRoot::GetMountIconLocation(	IconLocation&		rIconLocation,
											const std::wstring& rsPath,
											IPtrRefIconList		iconList,
											bool				bOpen) const
{
	IPtrPlisgoFSRoot root = PlisgoFSFolderReg::GetSingleton()->GetPlisgoFSRoot(rsPath.c_str());

	if (root.get() == NULL)
		return false;

	return  root->GetFolderIcon(rIconLocation, iconList->GetHeight(), bOpen);
}


bool	PlisgoFSRoot::GetShellInfoFolder(std::wstring& rsName, std::wstring& rsShellInfoFolder, const std::wstring& rsPath) const
{
	if (rsPath.length() < m_sPath.length())
		return false;

	size_t nSlash = rsPath.rfind(L'\\');

	if (nSlash != -1)
	{
		if (nSlash == rsPath.length()-1) //Ends with a slash
		{
			if (rsPath.length() == m_sPath.length())
				return false;

			nSlash = rsPath.rfind(L'\\', nSlash-1);

			rsName.assign(rsPath.begin()+1+nSlash, rsPath.end()-1);
		}
		else
		{
			if (rsPath.length() == m_sPath.length()-1) //-1 so length doesn't count trailing slash
				return false;

			rsName.assign(rsPath.begin()+1+nSlash, rsPath.end());
		}
	}
	else rsName = rsPath;


	rsShellInfoFolder = m_sPath;
	rsShellInfoFolder += L".plisgofs\\.shellinfo\\";

	if (nSlash > m_sPath.length())
	{
		const std::wstring sRelativePath(rsPath.begin() + m_sPath.length(), rsPath.begin() + nSlash);
	
		if (sRelativePath.length())
		{
			rsShellInfoFolder += sRelativePath;

			if (GetFileAttributes(rsShellInfoFolder.c_str()) == INVALID_FILE_ATTRIBUTES)
				return false;

			rsShellInfoFolder += L"\\";
		}
	}

	return true;
}


bool	PlisgoFSRoot::GetPathIconLocation(IconLocation& rIconLocation, const std::wstring& rsPath, IPtrRefIconList iconList, bool bOpen) const
{
	assert(iconList.get() != NULL);

	std::wstring sName;
	std::wstring sShellInfoFolder;

	if (!GetShellInfoFolder(sName, sShellInfoFolder, rsPath))
	{
		//There is no shell override here
		if (GetFileAttributes(rsPath.c_str()) & FILE_ATTRIBUTE_DIRECTORY)
			return iconList->GetFolderIconLocation(rIconLocation, bOpen);
		else
			return iconList->GetFileIconLocation(rIconLocation, rsPath);
	}

	const UINT nHeight = iconList->GetHeight();

	bool bBaseRetrieved = false;

	if (nHeight > 48)
		bBaseRetrieved = GetThumbnailIconLocation(rIconLocation, sShellInfoFolder, sName, nHeight);
	
	if (!bBaseRetrieved)
		bBaseRetrieved = GetCustomIconIconLocation(rIconLocation, sShellInfoFolder, sName, nHeight);

	if (!bBaseRetrieved)
	{
		if (GetFileAttributes(rsPath.c_str()) & FILE_ATTRIBUTE_DIRECTORY)
		{
			bBaseRetrieved = GetMountIconLocation(rIconLocation, rsPath, iconList, bOpen);

			if (!bBaseRetrieved)
				bBaseRetrieved = iconList->GetFolderIconLocation(rIconLocation, bOpen);
		}
		else
		{
			LPCWSTR sExt = NULL;

			size_t nPos = sName.rfind(L'.');

			if (nPos != -1)
				sExt = &sName.c_str()[nPos];

			bBaseRetrieved = GetExtensionIcon(rIconLocation, sExt, iconList->GetHeight());

			if (!bBaseRetrieved)
				bBaseRetrieved = iconList->GetFileIconLocation(rIconLocation, rsPath);
		}
	}
	
	if (!bBaseRetrieved)
		return false;

	IconLocation overlay;

	if (m_IconRegistry->ReadIconLocation(overlay, sShellInfoFolder + L".overlay_icons\\" + sName, iconList->GetHeight()))
	{
		IconLocation base = rIconLocation;

		if (!iconList->MakeOverlaid(rIconLocation, base, overlay))
			rIconLocation = base;
	}
	else
	{
		LPCWSTR sExt = NULL;

		size_t nPos = sName.rfind(L'.');

		if (nPos != -1)
			sExt = &sName.c_str()[nPos];

		if (sExt != NULL && (ExtIsShortcut(sExt) || ExtIsShortcutUrl(sExt)))
		{
			IconLocation overlay;

			overlay.sPath = L"shell32.dll";

			EnsureFullPath(overlay.sPath);

			overlay.nIndex = 29;

			IconLocation base = rIconLocation;

			if (!iconList->MakeOverlaid(rIconLocation, base, overlay))
				rIconLocation = base;
		}
	}

	return true;
}
