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
#include "IconRegistry.h"
#include "PlisgoFSFolder.h"

#include "../Common/Utils.h"

/*
	We both? Well both are used frequenty
*/
static void		GetPlisgoTempFolderW(std::wstring& rsResult)
{
	WCHAR sPlisgoFolder[MAX_PATH] = {0};

	GetTempPathW(MAX_PATH, sPlisgoFolder);

	wcscat_s(sPlisgoFolder, MAX_PATH, L"plisgo");

	if (GetFileAttributesW(sPlisgoFolder) == INVALID_FILE_ATTRIBUTES)
		CreateDirectoryW(sPlisgoFolder, NULL);

	rsResult = sPlisgoFolder;
}

static void		GetPlisgoTempFolderA(std::string& rsResult)
{
	char sPlisgoFolder[MAX_PATH] = {0};

	GetTempPathA(MAX_PATH, sPlisgoFolder);

	strcat_s(sPlisgoFolder, MAX_PATH, "plisgo");

	if (GetFileAttributesA(sPlisgoFolder) == INVALID_FILE_ATTRIBUTES)
		CreateDirectoryA(sPlisgoFolder, NULL);

	rsResult = sPlisgoFolder;
}


static bool		GetPlisgoFileLock(std::string& rsLockFile)
{
	GetPlisgoTempFolderA(rsLockFile);

	rsLockFile += "\\lock";

	if (GetFileAttributesA(rsLockFile.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		HANDLE hFile = CreateFileA(rsLockFile.c_str(), FILE_GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);

		if (hFile != NULL)
			CloseHandle(hFile);
		else
			return false;
	}

	return true;
}


static bool		GetIconFromImageListOfSize(IImageList* pImageList, HICON& rhDst, UINT nIndex, UINT nHeight)
{
	if (pImageList == NULL)
		return false;

	int nListWidth = -1, nListHeight = -1;

	if (pImageList->GetIconSize(&nListWidth, &nListHeight) != S_OK)
		return false;

	HICON hSrc = NULL;

	pImageList->GetIcon(nIndex, ILD_TRANSPARENT, &hSrc);
	
	rhDst = NULL;

	if (hSrc == NULL)
		return false;
	
	if (nListHeight != nHeight)
	{
		rhDst = (HICON)CopyImage(hSrc, IMAGE_ICON, nHeight, nHeight, 0);

		DestroyIcon(hSrc);

		return (rhDst != NULL);
	}
	else
	{
		rhDst = hSrc;

		return true;
	}
}


static void		GetLowerCaseExtension(std::wstring& rsExt, const std::wstring& rsPath)
{
	size_t nDot = rsPath.rfind(L'.');

	if (nDot == -1)
		nDot = 0;

	LPCWSTR sExt = &rsPath[nDot];

	if (nDot > 0 && ExtIsShortcut(sExt))
	{
		size_t nPrevDot = rsPath.rfind(L'.', nDot-1);

		rsExt.assign(&rsPath[nPrevDot], nDot-nPrevDot);
	}
	else rsExt = sExt;

	std::transform(rsExt.begin(),rsExt.end(),rsExt.begin(),tolower);
}


static ULONG64	GetLastModTime(LPCWSTR sFile)
{
	WIN32_FILE_ATTRIBUTE_DATA data;

	if (!GetFileAttributesExW(sFile, GetFileExInfoStandard, &data))
		return 0;

	return	*(ULONG64*)&data.ftLastWriteTime;
}

/*
*************************************************************************
*/
struct IconSource
{
	IconLocation	location;
	ULONG64			nLastModTime;

	void Init(const IconLocation& rLocation)
	{
		location = rLocation;

		if (rLocation.nFSIconRegID == -1)
			nLastModTime = GetLastModTime(rLocation.sPath.c_str());
		else
			nLastModTime = 0; // No point checking, only FS icon queries these and if the version is the same, it should all be present and the same.
	}
};





struct CreatedIcon
{
	friend class CreatedIconsRegistry;
public:

	void					CopyToIconLocation(IconLocation& rIconLocation)
	{
		ToWide(rIconLocation.sPath, m_sIconPath.c_str());
		rIconLocation.nIndex = 0;
	}


	bool					UpdateSourceTimes()
	{
		std::string sInfoFile;

		GetInfoFile(sInfoFile);

		std::fstream infoFile(sInfoFile.c_str(), std::ios_base::in | std::ios_base::out);

		std::string sLine;

		if (infoFile.is_open())
		{
			int nNotUsed;

			if (FindKeyLine(infoFile, sLine, nNotUsed))
			{
				//Wipe old line
				infoFile.seekp(-(std::fstream::pos_type)(sLine.length()+2), std::ios_base::cur); //The +2 is /r/n

				sLine.assign(sLine.length(), '-');

				infoFile.write(sLine.c_str(), sLine.length());

				infoFile.seekp(0, std::ios_base::end);
			}
		}
		else
		{
			infoFile.open(sInfoFile.c_str(), std::ios_base::out);

			if (!infoFile.is_open())
				return false;
		}

		if (!infoFile.good())
			infoFile.clear();

		sLine = m_sKey;
		sLine += m_sSourceTimes;
		sLine += m_sIconPath;

		infoFile << sLine;
		infoFile << std::endl;

		return true;
	}

	bool	Init(const std::vector<IconSource>& rSources, bool& rbIsCurrent)
	{
		if (rSources.size() == 0)
			return false;

		Init(rSources);

		std::string sLockFile;

		GetPlisgoFileLock(sLockFile);

		boost::interprocess::file_lock fileMutex(sLockFile.c_str());

		{
			boost::interprocess::sharable_lock<boost::interprocess::file_lock> lock(fileMutex);

			if (LoadFromFile(rbIsCurrent))
				return true;
		}

		{
			boost::interprocess::scoped_lock<boost::interprocess::file_lock> lock(fileMutex);

			return AlwaysLoadFromFile(rbIsCurrent);
		}
	}


protected:

	void					GetInfoFile(std::string& rsInfoFile)
	{
		GetPlisgoTempFolderA(rsInfoFile);

		rsInfoFile += (boost::format("\\%1%.txt") %m_nHash).str();
	}

	void					Init(const std::vector<IconSource>& rSources)
	{
		m_sIconPath.clear();

		std::wstring sKeyW;

		std::ostringstream ss;

		for(std::vector<IconSource>::const_iterator it = rSources.begin(); it != rSources.end(); ++it)
		{
			const IconLocation& rLocation = it->location;

			bool bFoundFSIconReg = false;

			if (rLocation.nFSIconRegID != -1 &&
				rLocation.nSrcIndex != -1 &&
				rLocation.nSrcList != -1)
			{
				IPtrFSIconRegistry reg = FSIconRegistriesManager::GetSingleton()->GetFSIconRegistry(rLocation.nFSIconRegID);

				if (reg.get() != NULL)
				{
					sKeyW += reg->GetFSName();
					sKeyW += (boost::wformat(L":%1%:%2%:%3%") %reg->GetFSVersion() %rLocation.nSrcList %rLocation.nSrcIndex).str();
					bFoundFSIconReg = true;
				}
			}
			
			if (!bFoundFSIconReg)
			{
				sKeyW += it->location.sPath;
				sKeyW += (boost::wformat(L":%1%") %it->location.nIndex).str();
			}

			ss << it->nLastModTime;
			ss << ';';
			sKeyW += L";";
		}

		boost::algorithm::to_lower(sKeyW);

		FromWide(m_sKey,sKeyW.c_str());

		m_nHash = boost::hash<std::string>()(m_sKey);
		
		m_sSourceTimes = ss.str();
	}


	bool					LoadFromFile(bool& rbCurrent)
	{
		std::string sInfoFile;

		GetInfoFile(sInfoFile);

		std::ifstream infoFile(sInfoFile.c_str());

		if (!infoFile.is_open())
			return false;

		int nNotUsed;

		return LoadFromStream(infoFile, rbCurrent, nNotUsed);
	}


	bool					AlwaysLoadFromFile(bool& rbCurrent)
	{
		std::string sInfoFile;

		GetInfoFile(sInfoFile);

		std::ifstream infoFile(sInfoFile.c_str());

		rbCurrent = false;

		int nEntryNum = 0;

		if (infoFile.is_open())
		{
			if (LoadFromStream(infoFile, rbCurrent, nEntryNum))
				return true;

			nEntryNum = nEntryNum;
		}

		
		GetPlisgoTempFolderA(m_sIconPath);

		m_sIconPath += (boost::format("\\%1%_%2%.ico") %m_nHash %nEntryNum).str();

		return true;
	}


private:

	template<class T>
	bool					LoadFromStream(T& s, bool& rbCurrent, int& rnEntryNum)
	{
		std::string sLine;

		if (!FindKeyLine(s, sLine, rnEntryNum))
			return false;

		size_t nIconPath = sLine.rfind(';',sLine.length());

		m_sIconPath.assign(&sLine.c_str()[nIconPath+1], sLine.length()-nIconPath);

		size_t nLineSourceTimesLen = nIconPath-m_sKey.length()+1;

		if (GetFileAttributesA(m_sIconPath.c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			if (m_sSourceTimes.length() == nLineSourceTimesLen)
				rbCurrent = (strncmp(m_sSourceTimes.c_str(), &sLine.c_str()[m_sKey.length()], nLineSourceTimesLen) == 0);
			else
				rbCurrent = false;
		}
		else rbCurrent = false;

		return true;
	}

	template<class T>
	bool FindKeyLine(T& s, std::string& rsLine, int& rnEntryNum)
	{
		rnEntryNum = 0;

		while(!s.eof())
		{
			std::getline(s, rsLine);

			if (rsLine[0] != '\0' && rsLine[0] != '-') //Lines can be blanked when updating.
			{
				if (strncmp(rsLine.c_str(), m_sKey.c_str(), m_sKey.length()) == 0)
				{
					return true;
				}

				++rnEntryNum;
			}
		}

		return false;
	}


	std::string			m_sIconPath;
	std::string			m_sKey;
	std::string			m_sSourceTimes;
	size_t				m_nHash;
};




/*
*************************************************************************
*/



bool	FSIconRegistry::LoadedImageList::Init(const std::wstring& rsFile)
{
	if (GetFileAttributes(rsFile.c_str()) == INVALID_FILE_ATTRIBUTES)
		return false;

	size_t nDot = rsFile.rfind(L'.');

	LPCWSTR sExt = rsFile.c_str()+nDot;

	if (ExtIsIconFile(sExt))
	{
		m_nFileType = 1;
	}
	else if (ExtIsCodeImage(sExt))
	{
		m_nFileType = 2;
	}
	else
	{
		m_hImageList = LoadImageList(rsFile);

		if (m_hImageList == NULL)
			return false;

		m_nFileType = 0;
	}

	m_sFile = rsFile;

	return true;
}


void	FSIconRegistry::LoadedImageList::Clear()
{
	if (m_hImageList != NULL)
		ImageList_Destroy(m_hImageList);
	
	m_sFile.clear();
}


bool	FSIconRegistry::CombiImageListIcon::Init(const WStringList& rFiles)
{
	bool bLoaded = false;

	for(WStringList::const_iterator it = rFiles.begin(); it != rFiles.end(); ++it)
	{
		UINT nHeight;

		size_t nBaseEnd = it->rfind(L'_');
		size_t nSlash = it->rfind(L'\\');

		size_t nIndexEnd = it->rfind(L'_', nBaseEnd-1);

		if (nIndexEnd != -1 && nIndexEnd > nSlash)
		{
			nHeight = (UINT)_wtol(it->c_str() + nBaseEnd+1);

			if (nHeight > 256)
				continue; //WHAT? I'm saving you from yourself (or errors).
		}
		else nHeight = 0; //There is no height, i.e single file that does all

		if (m_ImageLists.find(nHeight) != m_ImageLists.end())
			continue; //Loaded already..... two of the same height...some has been naughty.

		LoadedImageList& rLoad = m_ImageLists[nHeight];

		if (rLoad.Init(*it))
			bLoaded = true;
		else
			m_ImageLists.erase(nHeight);
	}

	return bLoaded;
}
		

void	FSIconRegistry::CombiImageListIcon::Clear()
{
	for(VersionedImageList::iterator it = m_ImageLists.begin();
		it != m_ImageLists.end(); ++it)
		it->second.Clear();
}


bool	FSIconRegistry::CombiImageListIcon::GetIconLocation(IconLocation& rIconLocation, UINT nIndex, bool& rbLoaded) const
{
	if (m_ImageLists.size() == 0)
		return false;

	const LoadedImageList& rLoaded = m_ImageLists.begin()->second;

	//Short cut special case
	if (m_ImageLists.size() == 1)
	{
		if (rLoaded.IsIconFile())
		{
			if (nIndex != 0)
				return false; //Invalid index, only 0 is invaid for icons

			rbLoaded = true;

			rIconLocation.sPath = rLoaded.GetFilePath();
			rIconLocation.nIndex = 0;

			return true;			
		}
		else if (rLoaded.IsCodeFile())
		{
			rbLoaded = true;

			rIconLocation.sPath = rLoaded.GetFilePath();
			rIconLocation.nIndex = (int)nIndex;

			return true;
		}		
	}


	if (nIndex >= m_Baked.size())
	{
		HIMAGELIST hImageList = rLoaded.GetImageList();

		if (nIndex >= (UINT)ImageList_GetImageCount(hImageList))
			return false; //This index is never valid

		//It's a valid index, but it's not loaded.
		rbLoaded = false;

		return true;
	}

	const IconLocation& rBurntIconLocation = m_Baked[nIndex];

	if (!rBurntIconLocation.IsValid())
	{
		//It's a valid index, but it's not loaded.
		rbLoaded = false;

		return true;
	}

	//It's already done, reuse it.
	rIconLocation = rBurntIconLocation;
	rbLoaded = true;

	return true;
}


bool	FSIconRegistry::CombiImageListIcon::CreateIconLocation(IconLocation& rIconLocation, UINT nList, UINT nIndex, UINT nFSRegId, IconRegistry* pIconRegistry)
{
	if (m_ImageLists.size() == 0)
		return false;

	{
		HIMAGELIST hImageList = m_ImageLists.begin()->second.GetImageList();

		if (nIndex >= (UINT)ImageList_GetImageCount(hImageList))
			return false; //This index is never valid
	}

	if (nIndex >= m_Baked.size())
		m_Baked.resize(nIndex+1);
	
	IconLocation& rBurntIconLocation = m_Baked[nIndex];

	std::vector<HICON>		subIcons;
	std::vector<IconSource>	iconSrcs;

	for(VersionedImageList::const_iterator it = m_ImageLists.begin(); it != m_ImageLists.end(); ++it)
	{
		HIMAGELIST hImageList = it->second.GetImageList();

		HICON hIcon = ImageList_GetIcon(hImageList, nIndex, ILD_TRANSPARENT);

		if (hIcon != NULL)
		{
			subIcons.push_back(hIcon);
			size_t nSrcIndex = iconSrcs.size();
			iconSrcs.resize(nSrcIndex+1);

			IconSource& rSrc = iconSrcs[nSrcIndex];

			rSrc.location.sPath = it->second.GetFilePath();
			rSrc.location.nIndex = nIndex;
			rSrc.location.nFSIconRegID = nFSRegId;
			rSrc.location.nSrcList = nList;
			rSrc.location.nSrcIndex = nIndex;
			rSrc.nLastModTime = 0; //Because it's FS we don't check time, more comments where used.
		}
	}


	assert(pIconRegistry != NULL);

	bool bIsCurrent = false;

	CreatedIcon createdIcon;

	if (!createdIcon.Init(iconSrcs, bIsCurrent))
		return false;

	createdIcon.CopyToIconLocation(rBurntIconLocation);

	if (bIsCurrent)
	{
		rIconLocation = rBurntIconLocation;

		return true;
	}

	bool bResult = WriteToIconFile(rBurntIconLocation.sPath, &subIcons[0], (int)subIcons.size(), false);

	if (bResult)
		createdIcon.UpdateSourceTimes();

	for(std::vector<HICON>::const_iterator it = subIcons.begin(); it != subIcons.end(); ++it)
		DestroyIcon(*it);

	rIconLocation = rBurntIconLocation;

	return bResult;
}

/*
*************************************************************************
						FSIconRegistry
*************************************************************************
*/

FSIconRegistry::FSIconRegistry( LPCWSTR				sFSName,
								int					nFSVersion,
								IconRegistry*		pMain,
								IPtrPlisgoFSRoot&	rRoot,
								UINT				nFSIconRegID)
{
	assert(pMain != NULL);

	m_sFSName		= sFSName;
	m_pMain			= pMain;
	m_nFSVersion	= nFSVersion;
	m_nFSIconRegID	= nFSIconRegID;

	AddReference(rRoot);
}


FSIconRegistry::~FSIconRegistry()
{
	for(CombiImageListIconMap::iterator it = m_ImageLists.begin();
		it != m_ImageLists.end(); ++it)
	{
		it->second.Clear();
	}
}


bool	FSIconRegistry::GetIconLocation(IconLocation& rIconLocation, UINT nList, UINT nIndex) const
{
	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);
	
	if (GetIconLocation_RLocked(rIconLocation, nList, nIndex))
		return true;
	
	boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

	if (GetIconLocation_RLocked(rIconLocation, nList, nIndex, true))
		return true;

	return const_cast<FSIconRegistry*>(this)->GetIconLocation_RWLocked(rIconLocation, nList, nIndex);
}


bool	FSIconRegistry::GetIconLocation_RLocked(IconLocation& rIconLocation, UINT nList, UINT nIndex, bool bWrite) const
{
	CombiImageListIconMap::const_iterator it = m_ImageLists.find(nList);

	if (it == m_ImageLists.end())
		return false;

	const CombiImageListIcon& rImageList = it->second;

	bool bLoaded = false;

	if (!rImageList.GetIconLocation(rIconLocation, nIndex, bLoaded))
		return false;
	
	if (bLoaded)
	{
		rIconLocation.nFSIconRegID	= m_nFSIconRegID;
		rIconLocation.nSrcList		= nList;
		rIconLocation.nSrcIndex		= nIndex;

		return true;
	}
	
	if (bWrite)
		return const_cast<CombiImageListIcon&>(rImageList).CreateIconLocation(rIconLocation, nList, nIndex, m_nFSIconRegID, m_pMain);
	else
		return false;
}


bool	FSIconRegistry::GetIconLocation_RWLocked(IconLocation& rIconLocation, UINT nList, UINT nIndex)
{
	std::wstring sBaseName = (boost::wformat(L".icons_%1%*") %nList).str();

	std::wstring sInstancePath;

	if (!GetInstancePath_Locked(sInstancePath))
		return false;

	std::wstring sTemp = sInstancePath + sBaseName;
	
	WIN32_FIND_DATA findData;

	HANDLE hFind = FindFirstFileW(sTemp.c_str(), &findData);

	if (hFind == NULL || hFind == INVALID_HANDLE_VALUE)
		return false;

	bool bDone = false;

	WStringList files;

	ULONG nBaseEnd = (ULONG)sBaseName.length()-3;

	do
	{
		std::wstring sFile = sInstancePath;
		sFile += findData.cFileName;

		if (findData.cFileName[nBaseEnd] == L'.')
		{
			files.clear();
			files.push_back(sFile);		

			bDone = true;

			break;
		}
		else if (findData.cFileName[nBaseEnd] == L'_')
			files.push_back(sFile);
	}
	while(FindNextFileW(hFind, &findData) != 0);

	FindClose(hFind);

	if (bDone)
		return true;

	CombiImageListIcon& rImageList = m_ImageLists[nList];

	if (!rImageList.Init(files))
	{
		m_ImageLists.erase(nList);

		return false;
	}

	return rImageList.CreateIconLocation(rIconLocation, nList, nIndex, m_nFSIconRegID, m_pMain);
}


bool	FSIconRegistry::ReadIconLocation(IconLocation& rIconLocation, const std::wstring& rsPligoFile) const
{
	HANDLE hFile = CreateFileW(rsPligoFile.c_str(), FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
		return false;
	
	CHAR sBuffer[MAX_PATH];

	DWORD nRead = 0;

	bool bResult = false;
	bool bCreated = false;


	if (ReadFile(hFile, sBuffer, MAX_PATH-1, &nRead, NULL) && nRead > 0)
	{
		if (isdigit(sBuffer[0]))
		{
			UINT nList, nIndex;
			
			if (ReadIndicesPair(sBuffer, nList, nIndex))	
			{
				bResult = GetIconLocation(rIconLocation, nList, nIndex);
				bCreated = true;

				if (bResult)
				{
					//NOTE: This is the only place a IconLocation's nSrcList and nSrcIndex could be set.
					rIconLocation.nFSIconRegID	= m_nFSIconRegID;
					rIconLocation.nSrcList		= nList;
					rIconLocation.nSrcIndex		= nIndex;
				}
			}
			else rIconLocation.nSrcList = rIconLocation.nSrcIndex = -1;
		}
		else
		{
			std::wstring sText;

			do
			{
				WCHAR sWBuffer[MAX_PATH];

				nRead = MultiByteToWideChar(CP_UTF8, 0, sBuffer, (int)nRead, sWBuffer, MAX_PATH);

				sWBuffer[nRead] = 0;

				sText += sWBuffer;
			}
			while(ReadFile(hFile, sBuffer, MAX_PATH-1, &nRead, NULL) && nRead > 0);

			if (sText.length())
			{
				const size_t nDevider = sText.find_last_of(L';');

				if (nDevider != -1)
					rIconLocation.nIndex = _wtoi(&sText.c_str()[nDevider]);
				else
					rIconLocation.nIndex = 0;

				rIconLocation.sPath.assign(sText.begin(),sText.begin()+nDevider);

				bResult = true;
			}
		}
	}

	CloseHandle(hFile);

	if (!bResult)
		return false;

	if (bCreated)
		return true;

	IconLocation nonWindowsIconLocation = rIconLocation;

	return m_pMain->GetWindowsIconLocation(rIconLocation, nonWindowsIconLocation);
}


bool	FSIconRegistry::GetInstancePath_Locked(std::wstring& rsResult)
{
	for(References::iterator it = m_References.begin(); it != m_References.end();)
	{
		IPtrPlisgoFSRoot reference = it->second.lock();

		if (reference.get() != NULL)
		{
			++it;

			if (reference->IsValid())
			{
				rsResult = reference->GetPlisgoPath();

				return true;
			}
		}
		else it = m_References.erase(it);
	}

	return false;
}


void	FSIconRegistry::AddReference(IPtrPlisgoFSRoot& rRoot)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	//Use the pointer as a GUID for the map
	m_References[(boost::uint64_t)rRoot.get()] = rRoot;
}


bool	FSIconRegistry::HasInstancePath()
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	for(References::iterator it = m_References.begin(); it != m_References.end(); ++it)
	{
		IPtrPlisgoFSRoot reference = it->second.lock();

		if (reference.get() != NULL && reference->IsValid())
			return true;
	}

	return false;
}


/*
*************************************************************************
						IconRegistry
*************************************************************************
*/
IconRegistry::IconRegistry()
{
	m_sShellPath = L"shell32.dll";
	EnsureFullPath(m_sShellPath);
}


bool			IconRegistry::GetFileIconLocation(IconLocation& rIconLocation, const std::wstring& rsPath) const
{
	if (ExtractOwnIconInfoOfFile(rIconLocation.sPath, rIconLocation.nIndex, rsPath))
	{
		//Has it's own icon

		return true;
	}

	size_t nExt = rsPath.rfind('.');

	if (nExt == -1)
		return false;

	LPCWSTR sExt = rsPath.c_str()+nExt;

	if (!ExtractIconInfoForExt( rIconLocation.sPath, rIconLocation.nIndex, sExt ))
	{
		rIconLocation.sPath = m_sShellPath;
		rIconLocation.nIndex = -1;
	}
/*
	SHFILEINFO info = {0};

	SHGetFileInfo(sExt, FILE_ATTRIBUTE_NORMAL, &info, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES|SHGFI_ICONLOCATION);

	if (info.szDisplayName[0] != L'\0')
		rIconLocation.sPath = info.szDisplayName;
	else
		rIconLocation.sPath = m_sShellPath;


	
	rIconLocation.nIndex = info.iIcon;*/

	return true;
}


bool			IconRegistry::GetFolderIconLocation(IconLocation& rIconLocation, bool bOpen) const
{
	if (bOpen)
	{
		rIconLocation.sPath = m_sShellPath;
		rIconLocation.nIndex = -5;
	}
	else
	{
		rIconLocation.sPath = m_sShellPath;
		rIconLocation.nIndex = -4;
	}

	return true;
}


bool			IconRegistry::GetDefaultOverlayIconLocation(IconLocation& rIconLocation, const std::wstring& rsName) const
{
	size_t nPos = rsName.rfind(L'.');

	if (nPos == -1)
		return false;

	LPCWSTR sExt = &rsName.c_str()[nPos];

	if (!ExtIsShortcut(sExt) && !ExtIsShortcutUrl(sExt))
		return false;

	rIconLocation.sPath = m_sShellPath;
	rIconLocation.nIndex = 29;

	return true;
}


bool			IconRegistry::GetAsWindowsIconLocation(IconLocation& rIconLocation, const std::wstring& rsImageFile, UINT nHeightHint) const
{
	std::vector<IconSource> srcs;

	srcs.resize(1);

	srcs[0].nLastModTime	= GetLastModTime(rsImageFile.c_str());
	srcs[0].location.sPath	= rsImageFile;
	srcs[0].location.nIndex = 0;

	bool bIsCurrent = false;

	
	CreatedIcon createdIcon;

	if (!createdIcon.Init(srcs, bIsCurrent))
		return false;

	createdIcon.CopyToIconLocation(rIconLocation);
	
	if (bIsCurrent)
		return true;

	if (nHeightHint == 0)
		nHeightHint = 256;

	ICONINFO newIcon = {0};

	newIcon.fIcon = TRUE;
	newIcon.hbmColor = ExtractBitmap(rsImageFile, nHeightHint, nHeightHint, 32);

	if (newIcon.hbmColor == NULL)
		return false;

	newIcon.hbmMask = CreateBitmap(nHeightHint,nHeightHint,1,1,NULL);

	HICON hIcon = CreateIconIndirect(&newIcon);

	bool bResult = false;

	if (hIcon != NULL)
		bResult = WriteToIconFile(rIconLocation.sPath, hIcon, false);

	if (bResult)
		createdIcon.UpdateSourceTimes();

	if (newIcon.hbmColor != NULL)
		DeleteObject(newIcon.hbmColor);

	if (newIcon.hbmMask != NULL)
		DeleteObject(newIcon.hbmMask);

	if (hIcon != NULL)
		DestroyIcon(hIcon);

	return bResult;
}


bool			IconRegistry::GetWindowsIconLocation(IconLocation& rIconLocation, const IconLocation& rSrcIconLocation, UINT nHeightHint) const
{
	size_t nDot = rSrcIconLocation.sPath.rfind(L'.');

	if (nDot == -1)
		return false;

	LPCWSTR sExt = rSrcIconLocation.sPath.c_str()+nDot;

	if (ExtIsIconFile(sExt) || ExtIsCodeImage(sExt))
	{
		rIconLocation = rSrcIconLocation;

		return true;
	}


	std::vector<IconSource> srcs;

	srcs.resize(1);

	srcs[0].Init(rSrcIconLocation);

	bool bIsCurrent = false;

	CreatedIcon createdIcon;

	if (!createdIcon.Init(srcs, bIsCurrent))
		return false;

	createdIcon.CopyToIconLocation(rIconLocation);
	
	if (bIsCurrent)
		return true;

	HICON hIcon;

	if (nHeightHint)
		hIcon = ExtractIconFromImageListFile(rSrcIconLocation.sPath, rSrcIconLocation.nIndex, nHeightHint);
	else
		hIcon = ExtractIconFromFile(rSrcIconLocation.sPath, rSrcIconLocation.nIndex);

	if (hIcon == NULL)
		return false;

	bool bResult = WriteToIconFile(rIconLocation.sPath, hIcon, false);

	if (bResult)
		createdIcon.UpdateSourceTimes();

	DestroyIcon(hIcon);
	
	return bResult;
}


bool			IconRegistry::MakeOverlaid(	IconLocation&		rDst,
											const IconLocation& rFirst,	
											const IconLocation& rSecond) const
{
	std::vector<IconSource> srcs;

	srcs.resize(2);

	srcs[0].Init(rFirst);
	srcs[1].Init(rSecond);

	bool bIsCurrent = false;

	CreatedIcon createdIcon;
	
	if (!createdIcon.Init(srcs, bIsCurrent))
		return false;

	createdIcon.CopyToIconLocation(rDst);

	if (bIsCurrent)
		return true;

	bool bResult = BurnIconsTogether(rDst.sPath.c_str(), rFirst.sPath.c_str(), rFirst.nIndex, rSecond.sPath.c_str(), rSecond.nIndex);

	if (bResult)
		createdIcon.UpdateSourceTimes();

	return bResult;
}

/*
*************************************************************************
						FSIconRegistriesManager
*************************************************************************
*/

IPtrFSIconRegistry	FSIconRegistriesManager::GetFSIconRegistry(LPCWSTR sFS, int nFSVersion, IPtrPlisgoFSRoot& rRoot) const
{
	IPtrFSIconRegistry result;

	if (sFS == NULL)
		return IPtrFSIconRegistry();

	std::wstring sKey = (boost::wformat(L"%1%:%2%") %sFS %nFSVersion).str();

	std::transform(sKey.begin(),sKey.end(),sKey.begin(), tolower);

	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	if (GetFSIconRegistry_Locked(result, sKey, rRoot))
		return result;

	boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

	if (GetFSIconRegistry_Locked(result, sKey, rRoot))
		return result;

	return const_cast<FSIconRegistriesManager*>(this)->CreateFSIconRegistry_Locked(sKey, sFS, nFSVersion, rRoot);
}

IPtrFSIconRegistry	FSIconRegistriesManager::GetFSIconRegistry(UINT nID) const
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	return m_FSIconRegistries[nID].lock();
}


bool	FSIconRegistriesManager::GetFSIconRegistry_Locked(IPtrFSIconRegistry& rResult, const std::wstring& rsKey, IPtrPlisgoFSRoot& rRoot) const
{
	IPtrFSIconRegistry result;

	FSIconRegistriesMap::const_iterator it = m_FSIconRegistriesMap.find(rsKey);

	if (it == m_FSIconRegistriesMap.end())
		return false;
	
	rResult = it->second.lock();

	if (rResult.get() == NULL)
		return false;
	
	rResult->AddReference(rRoot);

	return true;
}


IPtrFSIconRegistry	FSIconRegistriesManager::CreateFSIconRegistry_Locked(const std::wstring& rsKey, LPCWSTR sFS, int nFSVersion, IPtrPlisgoFSRoot& rRoot)
{
	IPtrFSIconRegistry result(new FSIconRegistry(sFS, nFSVersion, &m_IconRegistry, rRoot, (UINT)m_FSIconRegistries.size()));
	
	m_FSIconRegistriesMap[rsKey] = result;

	m_FSIconRegistries.push_back(result);

	return result;
}


FSIconRegistriesManager*	FSIconRegistriesManager::GetSingleton()
{
	static FSIconRegistriesManager singleton;

	return &singleton;
}