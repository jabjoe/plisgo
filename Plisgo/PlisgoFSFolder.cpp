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
#include "IconUtils.h"



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

	if (!ReadTextFromFile(m_sFSName, sPath + L".fsname") ||
		!ReadIntFromFile(m_nFSVersion, sPath + L".version") || m_nFSVersion != PLISGO_APIVERSION)
	{
		m_sFSName.resize(0);

		return;
	}

	//Ensure it could be used as a filename
	m_sFSName.erase(std::remove_if(m_sFSName.begin(), m_sFSName.end(), NotAlphaNumeric), m_sFSName.end());

	m_IconRegistry = pIconRegistry->GetFSIconRegistry(m_sFSName.c_str(), m_nFSVersion, sPath);

	std::wstring sTemp(sPath + L".column_header_*");

	WIN32_FIND_DATA findData;

	HANDLE hFind = FindFirstFileW(sTemp.c_str(), &findData);

	if (hFind != NULL && hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			std::wstring sHeader;
			
			ReadTextFromFile(sHeader, sPath + findData.cFileName);

			int nIndex = _wtoi(findData.cFileName + 15);//".column_header_"

			if (nIndex >= (int)m_Columns.size())
				m_Columns.resize(nIndex+1);

			ColumnDef& rColumnDef = m_Columns[nIndex];

			rColumnDef.sHeader = sHeader;

			std::wstring sTemp;

			WCHAR sBuffer[MAX_PATH];

			wsprintf(sBuffer, L".column_alignment_%i", nIndex);

			ReadTextFromFile(sTemp, sPath + sBuffer);

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

			ReadTextFromFile(sTemp, sPath + sBuffer);

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

			ReadIntFromFile(rColumnDef.nWidth, sPath + sBuffer);
		}
		while(FindNextFileW(hFind, &findData) != 0);

		FindClose(hFind);
	}

	sTemp = sPath + L".type_icons\\";

	if (!ReadIconIndices(	sTemp + L".folder_icon_closed",
							m_nFolderClosedIconImageList,
							m_nFolderClosedIconImageIndex))
	{
		m_nFolderClosedIconImageList = -1;
		m_nFolderClosedIconImageIndex = -1;
	}

	if (!ReadIconIndices(	sTemp + L".folder_icon_open",
							m_nFolderOpenIconImageList,
							m_nFolderOpenIconImageIndex))
	{
		m_nFolderOpenIconImageList = -1;
		m_nFolderOpenIconImageIndex = -1;
	}

	if (!ReadIconIndices(	sTemp + L".default_file",
							m_nFileDefaultIconImageList,
							m_nFileDefaultIconImageIndex))
	{
		m_nFileDefaultIconImageList = -1;
		m_nFileDefaultIconImageIndex = -1;
	}
	
	sTemp += L".*";

	hFind = FindFirstFileW(sTemp.c_str(), &findData);

	if (hFind != NULL && hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			UINT nList;
			UINT nIndex;

			//ever heard of longer then 4?
			//This is just a quick and easy way of ignoring .folder_icon_open, .folder_icon_closed and .default_file
			if (wcslen(findData.cFileName) > 6 && 
				ReadIconIndices(sPath + findData.cFileName, nList, nIndex))
			{
				WCHAR buffer[MAX_PATH];

				FillLowerCopy(buffer, MAX_PATH, &findData.cFileName[1]);

				m_ExtIconIndices[buffer] = std::pair<UINT,UINT>(nList, nIndex);
			}
		}
		while(FindNextFileW(hFind, &findData) != 0);

		FindClose(hFind);
	}
}


PlisgoFSRoot::~PlisgoFSRoot()
{
}


void	PlisgoFSRoot::GetMenuItems(IPtrPlisgoFSMenuList& rMenus, const WStringList& rSelection)
{
	PlisgoFSMenu::LoadMenus(rMenus, m_IconRegistry, m_sPath + L".plisgofs\\", rSelection);
}


bool	PlisgoFSRoot::GetFolderIcon(UINT& rnList, UINT& rnIndex, bool bOpen) const
{
	if (bOpen)
	{
		if (m_nFolderOpenIconImageList == -1 || m_nFolderOpenIconImageIndex == -1)
			return false;

		rnList = m_nFolderOpenIconImageList;
		rnIndex = m_nFolderOpenIconImageIndex;
	}
	else
	{
		if (m_nFolderClosedIconImageList == -1 || m_nFolderClosedIconImageIndex == -1)
			return false;

		rnList = m_nFolderClosedIconImageList;
		rnIndex = m_nFolderClosedIconImageIndex;
	}

	return true;
}


bool	PlisgoFSRoot::GetExtensionIcon(LPCWSTR sExt, UINT& rnList, UINT& rnIndex) const
{
	rnList = m_nFileDefaultIconImageList;
	rnIndex = m_nFileDefaultIconImageIndex;

	if (sExt == NULL || sExt[0] == L'\0')
		return (rnList != -1 && rnIndex != -1);

	WCHAR buffer[MAX_PATH];

	if (sExt[0] == L'.')
		++sExt;

	FillLowerCopy(buffer, MAX_PATH, sExt);

	ExtIconIndicesMap::const_iterator it = m_ExtIconIndices.find(buffer);

	if (it == m_ExtIconIndices.end())
		return (rnList != -1 && rnIndex != -1);

	rnList = it->second.first;
	rnIndex = it->second.second;

	return (rnList != -1 && rnIndex != -1);
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

	return ReadTextFromFile(rsResult, sPath);
}


bool	PlisgoFSRoot::GetPathColumnIntEntry(int& rnResult, const std::wstring& rsPath, int nIndex) const
{
	std::wstring sPath;

	if (!GetPathColumnEntryPath(sPath, rsPath, nIndex))
		return false;

	return ReadIntFromFile(rnResult, sPath);
}


bool	PlisgoFSRoot::GetPathColumnFloatEntry(double& rnResult, const std::wstring& rsPath, int nIndex) const
{
	std::wstring sPath;

	if (!GetPathColumnEntryPath(sPath, rsPath, nIndex))
		return false;

	return ReadDoubleFromFile(rnResult, sPath);
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
	std::wstring sFile;

	if (!GetThumbnailFile(sFile, rsShellInfoFolder, rsName))
		return false;

	WCHAR sBuffer[MAX_PATH];

	sBuffer[0] = L'\0';

	GetTempPath(MAX_PATH, sBuffer);

	WIN32_FILE_ATTRIBUTE_DATA data = {0};

	if (!GetFileAttributesEx(sFile.c_str(), GetFileExInfoStandard, &data))
		return false;

	const std::wstring sKey = (boost::wformat(L"%1%:%2%") %sFile %*(ULONG64*)&data.ftLastWriteTime).str();

	const ULONG64 nBaseHash = SimpleHash64(sKey.c_str());

	rIconLocation.sPath		= (boost::wformat(L"%1%plisgo_thumb_%2%_%3%_%4%_%5%.ico") %sBuffer %m_sFSName %m_nFSVersion %nBaseHash %nHeight).str();
	rIconLocation.nIndex	= 0;

	if (::GetFileAttributes(rIconLocation.sPath.c_str()) != INVALID_FILE_ATTRIBUTES)
		return true;

	HICON hIcon = LoadAsIcon(sFile, nHeight);

	if (hIcon == NULL)
		return false;
	
	return WriteToIconFile(rIconLocation.sPath, hIcon, false);
}


bool	PlisgoFSRoot::GetMountIconLocation(	IconLocation&		rIconLocation,
											const std::wstring& rsPath,
											IPtrRefIconList		iconList,
											bool				bOpen) const
{
	IPtrPlisgoFSRoot root = PlisgoFSFolderReg::GetSingleton()->GetPlisgoFSRoot(rsPath.c_str());

	if (root.get() == NULL)
		return false;

	IPtrFSIconRegistry FSIcons = root->GetFSIconRegistry();

	UINT nList, nIndex;

	if (!root->GetFolderIcon(nList, nIndex, bOpen))
		return false;

	return FSIcons->GetIconLocation(rIconLocation, nList, nIndex, iconList->GetHeight());
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
				sExt = &sName.c_str()[nPos+1];

			{
				UINT nList, nIndex;

				if (GetExtensionIcon(sExt, nList, nIndex))
					bBaseRetrieved = m_IconRegistry->GetIconLocation(rIconLocation, nList, nIndex, iconList->GetHeight());
			}

			if (!bBaseRetrieved)
				bBaseRetrieved = iconList->GetFileIconLocation(rIconLocation, rsPath);
		}
	}
	
	if (!bBaseRetrieved)
		return false;
	
	UINT nOverlayList	= -1;
	UINT nOverlayIndex	= -1;

	if (ReadIconIndices(sShellInfoFolder + L".overlay_icons\\" + sName, nOverlayList, nOverlayIndex))
	{
		IconLocation BaseLocation = rIconLocation;

		m_IconRegistry->GetIconLocation(rIconLocation, BaseLocation, iconList, nOverlayList, nOverlayIndex);
	}
	
	return true;
}
