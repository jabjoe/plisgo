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


#define NO_OVERLAYINDEX	0xFFFF
#define NO_OVERLAYLIST	0xFF


static void		GetPlisgoTempFolder(WCHAR sPlisgoFolder[MAX_PATH])
{
	GetTempPath(MAX_PATH, sPlisgoFolder);

	wcscat_s(sPlisgoFolder, MAX_PATH, L"plisgo");

	if (GetFileAttributes(sPlisgoFolder) == INVALID_FILE_ATTRIBUTES)
		CreateDirectory(sPlisgoFolder, NULL);
}


static bool		GetPlisgoFileLock(std::string& rsLockFile)
{
	WCHAR sPlisgoFolder[MAX_PATH];

	GetPlisgoTempFolder(sPlisgoFolder);

	FromWide(rsLockFile, sPlisgoFolder);

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


static bool		MatchesExistingIconInfoFile(const std::wstring& rsInfoFile, const std::vector<IconRegistry::IconSource>& rSources)
{
	std::string sLockFile;

	GetPlisgoFileLock(sLockFile);

	boost::interprocess::file_lock fileLock(sLockFile.c_str());

	boost::interprocess::sharable_lock<boost::interprocess::file_lock> lock(fileLock);

	HANDLE hFile = CreateFileW(rsInfoFile.c_str(), FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
		return false;

	bool bResult = true;

	ULONG32 nLength = 0;
	DWORD nRead = 0;

	if (ReadFile(hFile, &nLength, sizeof(ULONG32), &nRead, NULL) && nRead == sizeof(ULONG32))
	{
		if (nLength == (ULONG32)rSources.size())
		{
			//Note: Yes I do mean to check the order, because different order will mean things will be burn in different order

			std::wstring sPath;

			for(std::vector<IconRegistry::IconSource>::const_iterator it = rSources.begin(); it != rSources.end(); ++it)
			{
				if (!ReadFile(hFile, &nLength, sizeof(ULONG32), &nRead, NULL) || nRead != sizeof(ULONG32) ||
					nLength != it->location.sPath.length())
				{
					bResult = false;
					break;
				}

				sPath = it->location.sPath;

				if (!ReadFile(hFile, (WCHAR*)sPath.c_str(), nLength*sizeof(WCHAR), &nRead, NULL) || nRead != nLength*sizeof(WCHAR) ||
					!boost::algorithm::iequals(sPath, it->location.sPath))
				{
					bResult = false;
					break;
				}

				int nIndex;

				if (!ReadFile(hFile, &nIndex, sizeof(int), &nRead, NULL) || nRead != sizeof(int) ||
					nIndex != it->location.nIndex)
				{
					bResult = false;
					break;
				}

				ULONG64 nTime;

				if (!ReadFile(hFile, &nTime, sizeof(ULONG64), &nRead, NULL) || nRead != sizeof(ULONG64) ||
					!nTime != it->nLastModTime)
				{
					bResult = false;
					break;
				}
			}
		}
		else bResult = false;
	}
	else bResult = false;

	CloseHandle(hFile);

	return bResult;
}


static bool		WriteIconInfoFile(const std::wstring& rsInfoFile, const std::vector<IconRegistry::IconSource>& rSources)
{
	std::string sLockFile;

	GetPlisgoFileLock(sLockFile);

	boost::interprocess::file_lock fileLock(sLockFile.c_str());

	boost::interprocess::scoped_lock<boost::interprocess::file_lock> lock(fileLock);


	HANDLE hFile = CreateFileW(rsInfoFile.c_str(), FILE_WRITE_DATA, 0, NULL, OPEN_ALWAYS, 0, NULL);

	if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
		return false;

	bool bResult = true;

	DWORD nWritten = 0;
	ULONG32 nLength = (ULONG32)rSources.size();

	WriteFile(hFile, &nLength, sizeof(ULONG32), &nWritten, NULL);

	if (nWritten = sizeof(ULONG32))
	{
		for(std::vector<IconRegistry::IconSource>::const_iterator it = rSources.begin(); it != rSources.end(); ++it)
		{
			nLength = (ULONG32)it->location.sPath.length();

			if (!WriteFile(hFile, &nLength, sizeof(ULONG32), &nWritten, NULL) || nWritten != sizeof(ULONG32))
			{
				bResult = false;
				break;
			}

			if (!WriteFile(hFile, it->location.sPath.c_str(), nLength*sizeof(WCHAR), &nWritten, NULL) || nWritten != nLength*sizeof(WCHAR))
			{
				bResult = false;
				break;
			}

			if (!WriteFile(hFile, &it->location.nIndex, sizeof(int), &nWritten, NULL) || nWritten != sizeof(int))
			{
				bResult = false;
				break;
			}

			if (!WriteFile(hFile, &it->nLastModTime, sizeof(ULONG64), &nWritten, NULL) || nWritten != sizeof(ULONG64))
			{
				bResult = false;
				break;
			}
		}
	}
	else bResult = false;

	CloseHandle(hFile);

	return bResult;
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

/*
*************************************************************************
*/

static ULONG64	GetLastModTime(LPCWSTR sFile)
{
	WIN32_FILE_ATTRIBUTE_DATA data;

	if (!GetFileAttributesExW(sFile, GetFileExInfoStandard, &data))
		return 0;

	return	*(ULONG64*)&data.ftLastWriteTime;
}



bool	FSIconRegistry::LoadedImageList::Init(const std::wstring& rsFile)
{
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
	m_nLastModTime = ::GetLastModTime(rsFile.c_str());

	return true;
}


bool	FSIconRegistry::LoadedImageList::IsValid() const
{
	if (GetFileAttributes(m_sFile.c_str()) == INVALID_FILE_ATTRIBUTES)
		return false;

	return (::GetLastModTime(m_sFile.c_str()) == m_nLastModTime);
}


void	FSIconRegistry::LoadedImageList::Clear()
{
	if (m_hImageList != NULL)
		ImageList_Destroy(m_hImageList);
	
	m_sFile.clear();
}


bool	FSIconRegistry::CombiImageListIcon::IsValid() const
{
	for(VersionedImageList::const_iterator it = m_ImageLists.begin();
		it != m_ImageLists.end(); ++it)
	{
		if (!it->second.IsValid())
			return false;
	}

	return true;
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


bool	FSIconRegistry::CombiImageListIcon::CreateIconLocation(IconLocation& rIconLocation, UINT nIndex, IconRegistry* pIconRegistry)
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

	std::vector<HICON>						subIcons;
	std::vector<IconRegistry::IconSource>	iconSrcs;

	for(VersionedImageList::const_iterator it = m_ImageLists.begin(); it != m_ImageLists.end(); ++it)
	{
		HIMAGELIST hImageList = it->second.GetImageList();

		HICON hIcon = ImageList_GetIcon(hImageList, nIndex, ILD_TRANSPARENT);

		if (hIcon != NULL)
		{
			subIcons.push_back(hIcon);
			size_t nSrcIndex = iconSrcs.size();
			iconSrcs.resize(nSrcIndex+1);

			IconRegistry::IconSource& rSrc = iconSrcs[nSrcIndex];

			rSrc.location.sPath = it->second.GetFilePath();
			rSrc.location.nIndex = nIndex;
			rSrc.nLastModTime = it->second.GetLastModTime();
		}
	}


	assert(pIconRegistry != NULL);

	rBurntIconLocation.nIndex = 0;

	bool bResult = false;

	if (pIconRegistry->GetCreatedIconPath(rBurntIconLocation.sPath, iconSrcs))
		bResult = WriteToIconFile(rBurntIconLocation.sPath, &subIcons[0], (int)subIcons.size(), false);

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
								int					nVersion,
								IconRegistry*		pMain,
								IPtrPlisgoFSRoot&	rRoot)
{
	assert(pMain != NULL);

	m_sFSName	= sFSName;
	m_pMain		= pMain;
	m_nVersion	= nVersion;

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
	
	CombiImageListIconMap::const_iterator it = m_ImageLists.find(nList);

	if (it != m_ImageLists.end() && it->second.IsValid())
	{
		bool bLoaded = false;

		const CombiImageListIcon& rImageList = it->second;

		if (!rImageList.GetIconLocation(rIconLocation, nIndex, bLoaded))
			return false;
		
		if (bLoaded)
			return true;
	}
	
	boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

	it = m_ImageLists.find(nList);

	if (it != m_ImageLists.end())
	{
		CombiImageListIcon& rImageList = const_cast<CombiImageListIcon&>(it->second);

		if (rImageList.IsValid())
		{
			bool bLoaded = false;

			if (!rImageList.GetIconLocation(rIconLocation, nIndex, bLoaded))
				return false;
			
			if (bLoaded)
				return true;
			else
				return rImageList.CreateIconLocation(rIconLocation, nIndex, m_pMain);
		}
		
		rImageList.Clear();

		const_cast<FSIconRegistry*>(this)->m_ImageLists.erase(it);
	}

	return const_cast<FSIconRegistry*>(this)->GetIconLocation_Locked(rIconLocation, nList, nIndex);
}


bool	FSIconRegistry::GetIconLocation_Locked(IconLocation& rIconLocation, UINT nList, UINT nIndex)
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

	return rImageList.CreateIconLocation(rIconLocation, nIndex, m_pMain);
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
			UINT nList;
			UINT nIndex;

			if (ReadIndicesPair(sBuffer, nList, nIndex))	
			{
				bResult = GetIconLocation(rIconLocation, nList, nIndex);
				bCreated = true;
			}
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
		IPtrPlisgoFSRoot reference = it->lock();

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

	m_References.push_back(rRoot);
}


bool	FSIconRegistry::HasInstancePath()
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	for(References::iterator it = m_References.begin(); it != m_References.end(); ++it)
	{
		IPtrPlisgoFSRoot reference = it->lock();

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


bool			IconRegistry::CreatedIcon::IsValid() const
{
	if (GetFileAttributes(sResult.c_str()) == INVALID_FILE_ATTRIBUTES)
		return false;

	for(std::vector<IconSource>::const_iterator it = sources.begin(); it != sources.end(); ++it)
		if (it->nLastModTime != GetLastModTime(it->location.sPath.c_str()))
			return false;
		
	return true;
}


bool			IconRegistry::GetCreatedIconPath(std::wstring& rsIconPath, const std::vector<IconSource>& rSources, bool* pbIsCurrent) const
{
	if (rSources.size() == 0)
		return false; //No, this icon need to have come from somewhere.

	std::wstring sKey;

	for(std::vector<IconSource>::const_iterator it = rSources.begin(); it != rSources.end(); ++it)
	{
		sKey += it->location.sPath;
		sKey += (boost::wformat(L":%1%_") %it->location.nIndex).str();
	}

	boost::algorithm::to_lower(sKey);

	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	CreatedIcons::const_iterator it = m_CreatedIcons.find(sKey);

	if (it != m_CreatedIcons.end())
	{
		const CreatedIcon& rCreatedIcon = it->second;

		rsIconPath = rCreatedIcon.sResult;

		if (pbIsCurrent != NULL)
			*pbIsCurrent = rCreatedIcon.IsValid();

		return true;
	}
		
	boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

	it = m_CreatedIcons.find(sKey);

	if (it != m_CreatedIcons.end())
	{
		const CreatedIcon& rCreatedIcon = it->second;

		rsIconPath = rCreatedIcon.sResult;

		if (pbIsCurrent != NULL)
			*pbIsCurrent = rCreatedIcon.IsValid();

		return true;
	}

	WCHAR sPlisgoFolder[MAX_PATH];

	GetPlisgoTempFolder(sPlisgoFolder);

	ULONG64	nHash = boost::hash<std::wstring>()(sKey);//SimpleHash64(sKey.c_str());

	std::wstring sBaseFile = (boost::wformat(L"%1%\\%2%") %sPlisgoFolder %nHash).str();

	rsIconPath = sBaseFile + L".ico";

	CreatedIcon& rCreatedIcon = const_cast<IconRegistry*>(this)->m_CreatedIcons[sKey];

	rCreatedIcon.sResult = rsIconPath;
	rCreatedIcon.sources = rSources;

	std::wstring sInfoFile = sBaseFile + L".info";

	//Ok, not in the cache, but has another Plisgo instance from another process or session created it?

	if (MatchesExistingIconInfoFile(sInfoFile, rSources))
	{
		if (pbIsCurrent != NULL)
			*pbIsCurrent = true;
	}
	else
	{
		if (pbIsCurrent != NULL)
			*pbIsCurrent = false;
		
		WriteIconInfoFile(sInfoFile, rSources);
	}

	return true;
}


bool			IconRegistry::GetAsWindowsIconLocation(IconLocation& rIconLocation, const std::wstring& rsImageFile, UINT nHeightHint) const
{
	std::vector<IconSource> srcs;

	srcs.resize(1);

	srcs[0].nLastModTime	= GetLastModTime(rsImageFile.c_str());
	srcs[0].location.sPath	= rsImageFile;
	srcs[0].location.nIndex = 0;

	bool bValid = false;

	if (!GetCreatedIconPath(rIconLocation.sPath, srcs, &bValid))
		return false;
	
	if (bValid)
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

	rIconLocation.nIndex = 0;

	std::vector<IconSource> srcs;

	srcs.resize(1);

	srcs[0].nLastModTime	= GetLastModTime(rSrcIconLocation.sPath.c_str());
	srcs[0].location		= rSrcIconLocation;

	bool bValid = false;

	if (!GetCreatedIconPath(rIconLocation.sPath, srcs, &bValid))
		return false;
	
	if (bValid)
		return true;

	HICON hIcon;

	if (nHeightHint)
		hIcon = ExtractIconFromImageListFile(rSrcIconLocation.sPath, rSrcIconLocation.nIndex, nHeightHint);
	else
		hIcon = ExtractIconFromFile(rSrcIconLocation.sPath, rSrcIconLocation.nIndex);

	if (hIcon == NULL)
		return false;

	bool bResult = WriteToIconFile(rIconLocation.sPath, hIcon, false);

	DestroyIcon(hIcon);
	
	return bResult;
}


bool			IconRegistry::MakeOverlaid(	IconLocation&		rDst,
											const IconLocation& rFirst,	
											const IconLocation& rSecond) const
{
	std::vector<IconSource> srcs;

	srcs.resize(2);

	srcs[0].nLastModTime	= GetLastModTime(rFirst.sPath.c_str());
	srcs[0].location		= rFirst;

	srcs[1].nLastModTime	= GetLastModTime(rSecond.sPath.c_str());
	srcs[1].location		= rSecond;

	bool bIsCurrent = false;

	rDst.nIndex = 0;

	if (!GetCreatedIconPath(rDst.sPath, srcs, &bIsCurrent))
		return false;

	if (bIsCurrent)
		return true;

	return BurnIconsTogether(rDst.sPath.c_str(), rFirst.sPath.c_str(), rFirst.nIndex, rSecond.sPath.c_str(), rSecond.nIndex);
}

/*
*************************************************************************
						FSIconRegistriesManager
*************************************************************************
*/

IPtrFSIconRegistry	FSIconRegistriesManager::GetFSIconRegistry(LPCWSTR sFS, int nVersion, IPtrPlisgoFSRoot& rRoot) const
{
	IPtrFSIconRegistry result;

	if (sFS == NULL || nVersion < 1)
		return IPtrFSIconRegistry();

	std::wstring sKey = (boost::wformat(L"%1%:%2%") %sFS %nVersion).str();

	std::transform(sKey.begin(),sKey.end(),sKey.begin(), tolower);

	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	{
		FSIconRegistries::const_iterator it = m_FSIconRegistries.find(sKey);

		if (it != m_FSIconRegistries.end())
		{
			result = it->second.lock();

			if (result.get() != NULL)
			{
				result->AddReference(rRoot);

				return result;
			}
		}
	}

	boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

	return const_cast<FSIconRegistriesManager*>(this)->GetFSIconRegistry_Locked(sKey, sFS, nVersion, rRoot);
}



IPtrFSIconRegistry	FSIconRegistriesManager::GetFSIconRegistry_Locked(const std::wstring& rsKey, LPCWSTR sFS, int nVersion, IPtrPlisgoFSRoot& rRoot)
{
	IPtrFSIconRegistry result;

	FSIconRegistries::const_iterator it = m_FSIconRegistries.find(rsKey);

	if (it != m_FSIconRegistries.end())
	{
		result = it->second.lock();

		if (result.get() != NULL)
		{
			result->AddReference(rRoot);

			return result;
		}
	}

	result.reset(new FSIconRegistry(sFS, nVersion, &m_IconRegistry, rRoot));
	
	m_FSIconRegistries[rsKey] = result;

	return result;
}


FSIconRegistriesManager*	FSIconRegistriesManager::GetSingleton()
{
	static FSIconRegistriesManager singleton;

	return &singleton;
}