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


PlisgoFSRoot::PlisgoFSRoot()
{
	m_NameTime = 0;
}


void PlisgoFSRoot::Init(const std::wstring& rsPath)
{	
	m_sPath = rsPath;

	assert(m_sPath.size());

	if (*(m_sPath.end()-1) != L'\\')
		m_sPath += L'\\';

	m_sPlisgoPath = m_sPath + L".plisgofs\\";

	m_nAPIVersion = -1;

	std::wstring sNameFile = m_sPlisgoPath + L".fsname";

	if (!ReadTextFromFile(m_sFSName, sNameFile.c_str()) ||
		!ReadIntFromFile(m_nAPIVersion, (m_sPlisgoPath + L".version").c_str()) || m_nAPIVersion < 2) //v1 is no longer supported
	{
		m_sFSName.resize(0);

		return;
	}

	m_nFSVersion = 1;

	ReadIntFromFile(m_nFSVersion, (m_sPlisgoPath + L".fsversion").c_str());
	
	boost::trim_if(m_sFSName, boost::is_cntrl());

	WIN32_FILE_ATTRIBUTE_DATA data = {0};

	if (!GetFileAttributesEx(sNameFile.c_str(), GetFileExInfoStandard, (void*)&data))
	{
		m_sFSName.resize(0);

		return;
	}

	m_NameTime = (ULONG64&)data.ftCreationTime;


	//Ensure it could be used as a filename
	m_sFSName.erase(std::remove_if(m_sFSName.begin(), m_sFSName.end(), NotAlphaNumeric), m_sFSName.end());

	m_IconRegistry = FSIconRegistriesManager::GetSingleton()->GetFSIconRegistry(m_sFSName.c_str(), m_nFSVersion, shared_from_this());

	assert(m_IconRegistry.get() != NULL);

	std::wstring sTemp(m_sPlisgoPath + L".column_header_*");

	WIN32_FIND_DATA findData;

	HANDLE hFind = FindFirstFileW(sTemp.c_str(), &findData);

	if (hFind != NULL && hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			std::wstring sHeader;
			
			ReadTextFromFile(sHeader, (m_sPlisgoPath + findData.cFileName).c_str());

			int nIndex = _wtoi(findData.cFileName + 15);//".column_header_"

			if (nIndex >= (int)m_Columns.size())
				m_Columns.resize(nIndex+1);

			ColumnDef& rColumnDef = m_Columns[nIndex];

			rColumnDef.sHeader = sHeader;

			std::wstring sTemp;

			WCHAR sBuffer[MAX_PATH];

			wsprintf(sBuffer, L".column_alignment_%i", nIndex);

			ReadTextFromFile(sTemp, (m_sPlisgoPath + sBuffer).c_str());

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

			ReadTextFromFile(sTemp, (m_sPlisgoPath + sBuffer).c_str());

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

			ReadIntFromFile(rColumnDef.nWidth, (m_sPlisgoPath + sBuffer).c_str());
		}
		while(FindNextFileW(hFind, &findData) != 0);

		FindClose(hFind);
	}

	ZeroMemory(&m_DisabledStandardColumn, sizeof(m_DisabledStandardColumn));

	std::wstring sDisabledStandardColumns;
	
	ReadTextFromFile(sDisabledStandardColumns, (m_sPlisgoPath + L".disable_std_columns").c_str());

	if (sDisabledStandardColumns.length())
	{
		const WCHAR* sPos = sDisabledStandardColumns.c_str();

		while(sPos != NULL)
		{
			int nIndex = _wtoi(sPos);

			if (nIndex > 0 && nIndex < 7)
				m_DisabledStandardColumn[nIndex] = true;

			const WCHAR* sNext = wcschr(sPos, L',');

			if (sNext != NULL)
				++sNext;

			sPos = sNext;
		}
	}
}


PlisgoFSRoot::~PlisgoFSRoot()
{
}


void	PlisgoFSRoot::GetMenuItems(IPtrPlisgoFSMenuList& rMenus, const WStringList& rSelection)
{
	PlisgoFSMenu::LoadMenus(rMenus, m_IconRegistry, m_sPlisgoPath, rSelection);
}


bool	PlisgoFSRoot::GetFolderIcon(IconLocation& rIconLocation, bool bOpen ) const
{
	if (bOpen)
		return m_IconRegistry->ReadIconLocation(rIconLocation, (m_sPlisgoPath + L".type_icons\\.folder_icon_open").c_str());
	else
		return m_IconRegistry->ReadIconLocation(rIconLocation, (m_sPlisgoPath + L".type_icons\\.folder_icon_closed").c_str());
}


bool	PlisgoFSRoot::GetExtensionIcon(IconLocation& rIconLocation, LPCWSTR sExt) const
{
	if (sExt != NULL)
		if (m_IconRegistry->ReadIconLocation(rIconLocation, (m_sPath + L".plisgofs\\.type_icons\\" + sExt).c_str()))
			return true;
	
	return m_IconRegistry->ReadIconLocation(rIconLocation, (m_sPath + L".plisgofs\\.type_icons\\.default_file").c_str());

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


bool	PlisgoFSRoot::GetCustomIconIconLocation(IconLocation&		rIconLocation,
												const std::wstring& rsShellInfoFolder,
												const std::wstring& rsName) const
{
	return m_IconRegistry->ReadIconLocation(rIconLocation,
											rsShellInfoFolder + L".custom_icons\\" += rsName);
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
												const std::wstring& rsName) const
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
											bool				bOpen) const
{
	IPtrPlisgoFSRoot root = PlisgoFSFolderReg::GetSingleton()->GetPlisgoFSRoot(rsPath.c_str());

	if (root.get() == NULL)
		return false;

	return  root->GetFolderIcon(rIconLocation, bOpen);
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


bool	PlisgoFSRoot::GetThumbnailFile(	const std::wstring& rsPath,
										std::wstring&		rsResult) const
{
	std::wstring sName;
	std::wstring sShellInfoFolder;

	if (!GetShellInfoFolder(sName, sShellInfoFolder, rsPath))
		return false;

	return GetThumbnailFile(rsResult, sShellInfoFolder, sName);
}


bool	PlisgoFSRoot::GetFileOverlay(	const std::wstring& rsPath,
										IconLocation&		rIconLocation) const
{
	std::wstring sName;
	std::wstring sShellInfoFolder;

	if (!GetShellInfoFolder(sName, sShellInfoFolder, rsPath))
		return false;

	return m_IconRegistry->ReadIconLocation(rIconLocation, sShellInfoFolder + L".overlay_icons\\" + sName);
}


bool	PlisgoFSRoot::GetPathIconLocation(IconLocation& rIconLocation, const std::wstring& rsPath, bool bOpen) const
{
	std::wstring sName;
	std::wstring sShellInfoFolder;

	IconRegistry* pIconRegistry = FSIconRegistriesManager::GetSingleton()->GetIconRegistry();

	if (!GetShellInfoFolder(sName, sShellInfoFolder, rsPath))
	{
		//There is no shell override here
		if (GetFileAttributes(rsPath.c_str()) & FILE_ATTRIBUTE_DIRECTORY)
			return pIconRegistry->GetFolderIconLocation(rIconLocation, bOpen);
		else
			return pIconRegistry->GetFileIconLocation(rIconLocation, rsPath);
	}

	bool bBaseRetrieved = GetCustomIconIconLocation(rIconLocation, sShellInfoFolder, sName);

	if (!bBaseRetrieved)
	{
		if (GetFileAttributes(rsPath.c_str()) & FILE_ATTRIBUTE_DIRECTORY)
		{
			bBaseRetrieved = GetMountIconLocation(rIconLocation, rsPath, bOpen);

			if (!bBaseRetrieved)
				bBaseRetrieved = pIconRegistry->GetFolderIconLocation(rIconLocation, bOpen);
		}
		else
		{
			LPCWSTR sExt = NULL;

			size_t nPos = sName.rfind(L'.');

			if (nPos != -1)
				sExt = &sName.c_str()[nPos];

			bBaseRetrieved = GetExtensionIcon(rIconLocation, sExt);

			if (!bBaseRetrieved)
				bBaseRetrieved = pIconRegistry->GetFileIconLocation(rIconLocation, rsPath);
		}
	}
	
	if (!bBaseRetrieved)
		return false;

	IconLocation overlay;

	if (!m_IconRegistry->ReadIconLocation(overlay, (sShellInfoFolder + L".overlay_icons\\") += sName))
		pIconRegistry->GetDefaultOverlayIconLocation(overlay, sName);
	

	if (overlay.IsValid())
	{
		IconLocation base = rIconLocation;

		if (!pIconRegistry->MakeOverlaid(rIconLocation, base, overlay))
			rIconLocation = base;
	}

	return true;
}


bool	PlisgoFSRoot::IsValid() const
{
	const DWORD nAttr = GetFileAttributes(m_sPlisgoPath.c_str());

	if (nAttr == INVALID_FILE_ATTRIBUTES || !(nAttr & FILE_ATTRIBUTE_DIRECTORY))
		return false;

	std::wstring sNameFile = m_sPlisgoPath + L".fsname";

	WIN32_FILE_ATTRIBUTE_DATA data = {0};

	if (!GetFileAttributesEx(sNameFile.c_str(), GetFileExInfoStandard, (void*)&data))
		return false;

	if (m_nAPIVersion > 2) //It was 3 that string files started storing time
		if (m_NameTime != (ULONG64&)data.ftCreationTime)
			return false;

	int nAPIVersion = -1;
	std::wstring sName;

	if (!ReadTextFromFile(sName, sNameFile.c_str()) ||
		!ReadIntFromFile(nAPIVersion, (m_sPlisgoPath + L".version").c_str()))
	{
		return false;
	}

	if (nAPIVersion != m_nAPIVersion)
		return false;

	boost::trim_if(sName, boost::is_cntrl());

	if (!boost::algorithm::iequals(sName, m_sFSName))
		return false;

	return true;
}