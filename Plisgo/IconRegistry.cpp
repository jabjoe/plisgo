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
#include "IconUtils.h"


#define NO_OVERLAYINDEX	0xFFFF
#define NO_OVERLAYLIST	0xFF


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

/*
*************************************************************************
						FSIconRegistry
*************************************************************************
*/

FSIconRegistry::FSIconRegistry( LPCWSTR				sFSName,
								int					nVersion,
								IconRegistry*		pMain,
								const std::wstring& rsRootPath)
{
	assert(pMain != NULL);

	m_sFSName	= sFSName;
	m_pMain		= pMain;
	m_nVersion	= nVersion;

	std::wstring sPath = rsRootPath;

	if (sPath.length() && *(sPath.end()-1) != L'\\')
		sPath += L'\\';

	AddInstancePath(sPath);

	std::wstring sTemp(sPath + L".icons_*_*.*");

	WIN32_FIND_DATA findData;

	HANDLE hFind = FindFirstFileW(sTemp.c_str(), &findData);

	if (hFind != NULL && hFind != INVALID_HANDLE_VALUE)
	{
		std::vector<WStringList>	iconFiles;

		do
		{
			if (isdigit(findData.cFileName[7]) == 0)
				continue;			

			const size_t nIndex = _wtoi(&findData.cFileName[7]);//len(".icons_")

			if (nIndex == -1)
				continue;

			WCHAR* sHeight = wcsrchr(findData.cFileName,L'_');

			if (sHeight == NULL)
				continue;

			sHeight++;

			if (isdigit(*sHeight) == 0)
				continue;			

			UINT nHeight = _wtoi(sHeight);
		
			while(nIndex >= m_ImageLists.size() )
				m_ImageLists.push_back(VersionedImageList());
		
			m_ImageLists[nIndex].push_back(VersionImageList(nHeight));
		}
		while(FindNextFileW(hFind, &findData) != 0);

		FindClose(hFind);
	}
}



bool	FSIconRegistry::GetBestImageList_RO(VersionedImageList::const_iterator& rBestFit, UINT nList, UINT nHeight) const
{
	if (nList >= m_ImageLists.size())
		return false;

	const VersionedImageList& rVersionedList = m_ImageLists[nList];

	if (rVersionedList.size() == 0)
		return false;

	VersionedImageList::const_iterator it = rVersionedList.begin();
	
	rBestFit = it;

	if (it != rVersionedList.end())
	{
		do
		{
			if (it->nHeight == nHeight)
			{
				rBestFit = it;
				break;
			}
			else if ( (rBestFit->nHeight > nHeight && it->nHeight < rBestFit->nHeight && it->nHeight > nHeight)
									||
					 (rBestFit->nHeight < nHeight && it->nHeight > rBestFit->nHeight) )
			{
				rBestFit = it;
			}

			it++;
		}
		while(it != rVersionedList.end());
	}

	return true;
}


bool	FSIconRegistry::GetBestImageList_RW(VersionedImageList::const_iterator& rBestFit, UINT nList, UINT nHeight)
{
	if (nList >= m_ImageLists.size())
		return false;

	VersionedImageList& rVersionedList = m_ImageLists[nList];

	if (rVersionedList.size() == 0)
		return false;

	if (rBestFit->ImageList != NULL)
		return true;
	
	//Check address
	if (rBestFit < rVersionedList.begin() && rBestFit >= rVersionedList.end())
		return false;
	
	const size_t nBestFitIndex = rBestFit-rVersionedList.begin();

	VersionImageList& rEntry = const_cast<FSIconRegistry*>(this)->m_ImageLists[nList][nBestFitIndex];

	if (rEntry.ImageList != NULL) //If we used rBestFit it might be what's in cache (due to const) and be out of date.
	{
		rBestFit = rVersionedList.begin()+nBestFitIndex;

		return true;
	}

	const std::wstring& rsPath = const_cast<FSIconRegistry*>(this)->GetInstancePath_RW();

	std::wstring sTemp = (boost::wformat(L"%1%.icons_%2%_%3%.*") %rsPath %nList %rBestFit->nHeight).str();
	
	WIN32_FIND_DATA findData;

	HANDLE hFind = FindFirstFileW(sTemp.c_str(), &findData);

	if (hFind != NULL && hFind != INVALID_HANDLE_VALUE)
	{
		std::wstring sFile(rsPath + findData.cFileName);

		FindClose(hFind);

		HIMAGELIST hImageList = LoadImageList(sFile, rBestFit->nHeight);

		if (hImageList != NULL)
		{
			rEntry.ImageList = reinterpret_cast<IImageList*>(hImageList);
			rBestFit = rVersionedList.begin()+nBestFitIndex; //Force cache refresh

			ImageList_Destroy(hImageList); //Dec ref count

			return true;
		}
	}

	//It's hosed, remove it.
	const_cast<FSIconRegistry*>(this)->m_ImageLists[nList].erase(rBestFit);

	//Try again
	if (!GetBestImageList_RO(rBestFit, nList, nHeight))
		return false;
	
	if (rBestFit->ImageList == NULL)
		return GetBestImageList_RW(rBestFit, nList, nHeight);
	else
		return true;
}


void		FSIconRegistry::AddInstancePath(std::wstring sRootPath)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	bool bFound = false;

	std::transform(sRootPath.begin(),sRootPath.end(),sRootPath.begin(), tolower);

	if (sRootPath.length() && *(sRootPath.end()-1) != L'\\')
		sRootPath += L'\\';

	//Trim dead paths
	for(WStringList::iterator it = m_Paths.begin(); it != m_Paths.end();)
		if (GetFileAttributes((*it).c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			if (*it == sRootPath)
				bFound = true;
			++it;
		}
		else it = m_Paths.erase(it);

	if (!bFound)
		m_Paths.push_back(sRootPath);
}


const std::wstring&		FSIconRegistry::GetInstancePath_RW()
{
	//Trim dead paths
	for(WStringList::iterator it = m_Paths.begin(); it != m_Paths.end();)
	{
		if (GetFileAttributes((*it).c_str()) == INVALID_FILE_ATTRIBUTES)
			it = m_Paths.erase(it);
		else
			++it;
	}

	assert(m_Paths.begin() != m_Paths.end());

	return *m_Paths.begin();
}



bool		FSIconRegistry::GetFSIcon(HICON& rhIcon, UINT nList, UINT nIndex, UINT nHeight) const
{
	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	VersionedImageList::const_iterator itBestFit;

	if (!GetBestImageList_RO(itBestFit, nList, nHeight))
		return false;

	if (itBestFit->ImageList == NULL)
	{
		boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

		if (!const_cast<FSIconRegistry*>(this)->GetBestImageList_RW(itBestFit, nList, nHeight))
			return false;
	}

	return GetIconFromImageListOfSize(itBestFit->ImageList, rhIcon, nIndex, nHeight);
}


bool		FSIconRegistry::GetFSIcon_RW(HICON& rhIcon, UINT nList, UINT nIndex, UINT nHeight)
{
	VersionedImageList::const_iterator itBestFit;

	if (!GetBestImageList_RO(itBestFit, nList, nHeight))
		return false;

	if (itBestFit->ImageList == NULL)
		if (!const_cast<FSIconRegistry*>(this)->GetBestImageList_RW(itBestFit, nList, nHeight))
			return false;

	return GetIconFromImageListOfSize(itBestFit->ImageList, rhIcon, nIndex, nHeight);
}


bool		FSIconRegistry::ReadIconLocation(IconLocation& rIconLocation, const std::wstring& rsPligoFile, const ULONG nHeight)
{
	HANDLE hFile = CreateFileW(rsPligoFile.c_str(), FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
		return false;
	
	CHAR sBuffer[MAX_PATH];

	DWORD nRead = 0;

	bool bResult = false;


	if (ReadFile(hFile, sBuffer, MAX_PATH-1, &nRead, NULL) && nRead > 0)
	{
		if (isdigit(sBuffer[0]))
		{
			UINT nList;
			UINT nIndex;

			if (ReadIndicesPair(sBuffer, nList, nIndex))			
				bResult = GetIconLocation(rIconLocation, nList, nIndex, nHeight);	
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

				SetFilePointer(hFile, nRead, 0, FILE_CURRENT);
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

	return bResult;
}


bool		FSIconRegistry::GetIconLocation(IconLocation& rIconLocation, UINT nList, UINT nIndex, const ULONG nHeight) const
{
	int nResult = DEFAULTFILEICONINDEX;

	if (nList == -1 || nIndex == -1 )
		return false;

	const std::wstring sKey = (boost::wformat(L"%1%:%2%:%3%") %nList %nIndex %nHeight).str();
	
	WCHAR sBuffer[MAX_PATH];

	sBuffer[0] = L'\0';

	GetTempPath(MAX_PATH, sBuffer);

	rIconLocation.nIndex	= 0;
	
	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	bool bResult = true;

	if (!LookupKey(rIconLocation.sPath, sKey))
	{
		boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

		if (!LookupKey(rIconLocation.sPath, sKey)) //Check nothing has changed getting rw lock
		{
			rIconLocation.sPath	= (boost::wformat(L"%1%plisgo_cache_%2%_%3%_%4%_%5%_%6%.ico") %sBuffer %m_sFSName %m_nVersion %nHeight %nList %nIndex).str();
	
			HICON hBaseImage;

			bResult = false;

			if (const_cast<FSIconRegistry*>(this)->GetFSIcon_RW(hBaseImage, nList, nIndex, nHeight))
			{
				if (WriteToIconFile(rIconLocation.sPath, hBaseImage, false))
				{
					const_cast<FSIconRegistry*>(this)->m_BurnReadyCached[sKey] = rIconLocation.sPath;
					bResult = true;
				}

				if (hBaseImage != NULL)
					DestroyIcon(hBaseImage);
			}
		}
	}
	

	return bResult;
}


bool		FSIconRegistry::LookupKey(std::wstring& rsPath, const std::wstring& rsKey) const
{
	BurnReadyCached::const_iterator it = m_BurnReadyCached.find(rsKey);

	if (it == m_BurnReadyCached.end())
		return false;

	rsPath = it->second;

	return true;
}


bool		FSIconRegistry::GetIconLocation(IconLocation& rIconLocation, const IconLocation& rBase, IPtrRefIconList iconList, UINT nOverlayList, UINT nOverlayIndex) const
{
	if (!rBase.IsValid() || nOverlayList == -1 || nOverlayIndex == -1) 
		return false;

	const UINT nHeight = iconList->GetHeight();

	const std::wstring sKey = (boost::wformat(L"%1%:%2%:%3%:%4%:%5%") %rBase.sPath %rBase.nIndex %nOverlayList %nOverlayIndex %nHeight).str();

	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	rIconLocation.nIndex	= 0;

	bool bResult = true;

	if (!LookupKey(rIconLocation.sPath, sKey))
	{
		boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

		if (!LookupKey(rIconLocation.sPath, sKey))
		{
			bResult = false;

			const std::wstring	sBaseHashKey	= (boost::wformat(L"%1%:%2%") %rBase.sPath %rBase.nIndex ).str();
			const ULONG64		nBaseHash		= SimpleHash64(sBaseHashKey.c_str());
	
			WCHAR sBuffer[MAX_PATH];

			sBuffer[0] = L'\0';

			GetTempPath(MAX_PATH, sBuffer);

			rIconLocation.sPath	= (boost::wformat(L"%1%plisgo_cache_%2%_%3%_%4%_%5%_%6%_%7%.ico") %sBuffer %m_sFSName %m_nVersion %nHeight %nBaseHash %nOverlayList %nOverlayIndex).str();

			HICON hOverlay = NULL;

			if (const_cast<FSIconRegistry*>(this)->GetFSIcon_RW(hOverlay, nOverlayList, nOverlayIndex, nHeight))
			{
				HICON hBase = NULL;

				int nStdIndex;

				if (iconList->GetIconLocationIndex(nStdIndex, rBase) &&
					iconList->GetIcon(hBase, nStdIndex))
				{
					HICON hBoth = BurnTogether(hBase, hOverlay, nHeight);

					if (hBoth != NULL)
					{
						if (WriteToIconFile(rIconLocation.sPath, hBoth, false))
						{
							const_cast<FSIconRegistry*>(this)->m_BurnReadyCached[sKey] = rIconLocation.sPath;
							bResult = true;
						}

						DestroyIcon(hBoth);
					}

					DestroyIcon(hBase);
				}

				DestroyIcon(hOverlay);
			}
		}
	}
	

	return bResult;
}


/*
*************************************************************************
						RefIconList
*************************************************************************
*/

RefIconList::RefIconList(UINT nHeight)
{
	m_nLastOldestEntry = -1;

	m_nHeight = nHeight;

	HIMAGELIST hImageList = ImageList_Create(nHeight, nHeight, ILC_MASK | ILC_COLOR32, 0, 4);

	m_ImageList = reinterpret_cast<IImageList*>(hImageList);

	ImageList_Destroy(hImageList); //Dec ref count

	IconLocation Location;

	Location.sPath = L"shell32.dll";

	EnsureFullPath(Location.sPath);

	int nTemp;

	Location.nIndex = 0;
	GetIconLocationIndex(nTemp, Location); //DEFAULTFILEICONINDEX
	Location.nIndex = -4;
	GetIconLocationIndex(nTemp, Location); //DEFAULTCLOSEDFOLDERICONINDEX
	Location.nIndex = -5;
	GetIconLocationIndex(nTemp, Location); //DEFAULTOPENFOLDERICONINDEX
}


bool		RefIconList::GetFileIconLocation(IconLocation& rIconLocation, const std::wstring& rsPath) const
{
	size_t nDot = rsPath.rfind(L'.');

	if (nDot != -1)
	{
		LPCWSTR sExt = &rsPath[nDot];

		if (ExtIsShortcut(sExt) || ExtIsCodeImage(sExt) || ExtIsIconFile(sExt))
		{
			rIconLocation.sPath		= rsPath;
			rIconLocation.nIndex	= 0;

			int nTemp;

			if (GetIconLocationIndex(nTemp, rIconLocation))
				return true;
		}
	}
	else nDot = 0;

	size_t nLen = rsPath.length()-nDot;

	std::wstring sKey(nLen,L' ');

	std::transform(rsPath.begin()+nDot,rsPath.end(),sKey.begin(),tolower);


	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	CachedIconsMap::const_iterator it = m_Extensions.find(sKey);

	if (it != m_Extensions.end())
	{
		if (it->second > -1)
		{
			rIconLocation = m_CachedIcons[it->second].Location;

			return rIconLocation.IsValid();
		}
		else
		{
			rIconLocation = m_CachedIcons[DEFAULTFILEICONINDEX].Location;

			return rIconLocation.IsValid();
		}
	}
	
	boost::upgrade_to_unique_lock<boost::shared_mutex> rwlock(lock);

	it = m_Extensions.find(sKey);

	if (it != m_Extensions.end())
	{
		if (it->second > -1)
		{
			rIconLocation = m_CachedIcons[it->second].Location;

			return rIconLocation.IsValid();
		}
		else
		{
			rIconLocation = m_CachedIcons[DEFAULTFILEICONINDEX].Location;

			return rIconLocation.IsValid();
		}
	}
	
	int nResult = DEFAULTFILEICONINDEX;

	if (ExtractIconInfoForExt(rIconLocation.sPath, rIconLocation.nIndex, sKey.c_str()))
	{
		std::wstring sEntryKey = (boost::wformat(L"%1%:%2%") %rIconLocation.sPath %rIconLocation.nIndex).str();

		std::transform(sEntryKey.begin(),sEntryKey.end(),sEntryKey.begin(),tolower);

		const_cast<RefIconList*>(this)->GetIconLocationIndex_RW(nResult, rIconLocation, sEntryKey);
	}
	else rIconLocation = m_CachedIcons[DEFAULTFILEICONINDEX].Location;

	if (nResult != -1 && nResult != DEFAULTFILEICONINDEX)
	{
		const_cast<RefIconList*>(this)->m_Extensions[sKey] = nResult;
		const_cast<RefIconList*>(this)->m_ExtensionsInverse[nResult] = sKey;
	}
	else
	{
		const_cast<RefIconList*>(this)->m_Extensions[sKey] = DEFAULTFILEICONINDEX;
		rIconLocation = m_CachedIcons[DEFAULTFILEICONINDEX].Location;
	}

	return rIconLocation.IsValid();
}


bool		RefIconList::AddEntry_RW(int& rnIndex, HICON hIcon)
{
	assert(hIcon != NULL);

	int	nFreeIndex = GetFreeSlot();

	if (nFreeIndex == -1)
		return false;
	
	if (m_ImageList->ReplaceIcon(nFreeIndex, hIcon, &rnIndex) != S_OK || rnIndex != nFreeIndex)
	{
		m_EntryUsed[nFreeIndex] = false;

		return false;
	}

	return true;
}


int			RefIconList::GetFreeSlot()
{
	for(std::vector<bool>::iterator it = m_EntryUsed.begin();
		it != m_EntryUsed.end(); ++it)
	{
		if (*it == false)
		{
			*it = true;
			return it-m_EntryUsed.begin();
		}
	}

	int nResult = 0;

	m_ImageList->GetImageCount(&nResult);

	
	if (m_ImageList->SetImageCount(nResult+1) != S_OK)
	{
		m_ImageList->SetImageCount(nResult);

		return -1;
	}

	m_EntryUsed.push_back(true);

	return nResult;
}


bool		RefIconList::GetFolderIconLocation(IconLocation& rIconLocation, bool bOpen) const
{
	const CachedIcon& rCachedIcon = m_CachedIcons[(bOpen)?DEFAULTOPENFOLDERICONINDEX:DEFAULTCLOSEDFOLDERICONINDEX];

	rIconLocation = rCachedIcon.Location;

	return true;
}


bool		RefIconList::GetIconLocation(IconLocation& rIconLocation, const int nIndex) const
{
	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	if ((unsigned int)nIndex >= m_CachedIcons.size())
		return false;

	if (!m_EntryUsed[nIndex])
		return false;

	int nImageCount = 0;

	assert(m_ImageList->GetImageCount(&nImageCount) == S_OK && (unsigned int)nImageCount == m_CachedIcons.size());

	const CachedIcon &rCachedIcon = m_CachedIcons[nIndex];

	rIconLocation = rCachedIcon.Location;

	GetSystemTimeAsFileTime((FILETIME*)&rCachedIcon.nTime);

	//InterlockedExchange64 can not be used, at least not on this machine.

	return true;
}


bool		RefIconList::GetIcon(HICON& rhResult, const int nIndex) const
{
	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	assert(nIndex < (int)m_CachedIcons.size());

	GetSystemTimeAsFileTime((FILETIME*)&m_CachedIcons[nIndex].nTime);

	return (m_ImageList->GetIcon(nIndex, ILD_TRANSPARENT, &rhResult) == S_OK);
}


bool		RefIconList::GetIconLocationIndex(int& rnIndex, const IconLocation& rIconLocation) const
{
	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	std::wstring sKey = (boost::wformat(L"%1%:%2%") %rIconLocation.sPath %rIconLocation.nIndex).str();

	std::transform(sKey.begin(),sKey.end(),sKey.begin(),tolower);

	CachedIconsMap::const_iterator it = m_CachedIconsMap.find(sKey);

	if (it != m_CachedIconsMap.end())
	{
		rnIndex = it->second;

		if (rnIndex >= 0 && m_CachedIcons[rnIndex].IsValid())
			return true;
	}

	boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

	return const_cast<RefIconList*>(this)->GetIconLocationIndex_RW(rnIndex, rIconLocation, sKey);
}


bool		RefIconList::GetIconLocationIndex_RW(int& rnIndex, const IconLocation& rIconLocation, const std::wstring& rsKey)
{
	CachedIconsMap::const_iterator it = m_CachedIconsMap.find(rsKey);

	if (it != m_CachedIconsMap.end())
	{
		rnIndex = it->second;

		if (rnIndex >= 0 && m_CachedIcons[rnIndex].IsValid())
			return true;
	}

	HICON hResult = GetSpecificIcon(rIconLocation.sPath, rIconLocation.nIndex, m_nHeight);

	if (hResult == NULL)
	{
		m_CachedIconsMap[rsKey] = -1;

		return false;
	}

	bool bResult = false;

	rnIndex = -1;

	if (AddEntry_RW(rnIndex, hResult))
	{
		bResult = true;

		CachedIcon temp;

		temp.nTime = 0;

		//Catch up size, should really only ever be 1 behind
		while(rnIndex+1 > (int)m_CachedIcons.size())
			m_CachedIcons.push_back(temp);

		assert (rnIndex < (int)m_CachedIcons.size());
		
		CachedIcon& rCachedIcon = m_CachedIcons[rnIndex];
		
		rCachedIcon.Location = rIconLocation;

		GetSystemTimeAsFileTime((FILETIME*)&rCachedIcon.nTime);

		WIN32_FILE_ATTRIBUTE_DATA data = {0};

		GetFileAttributesEx(rIconLocation.sPath.c_str(), GetFileExInfoStandard, &data);
		
		rCachedIcon.nLastModified = *(ULONG64*)&data.ftLastWriteTime;
	}

	m_CachedIconsMap[rsKey] = rnIndex;

	DestroyIcon(hResult);

	return bResult;
}


void		RefIconList::RemoveOlderThan(ULONG64 n100ns)
{
	ULONG64 nNow;

	GetSystemTimeAsFileTime((FILETIME*)&nNow);

	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);


	if (nNow - m_nLastOldestEntry <= n100ns)
		return; //Nothing to do


	{
		boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

		if (nNow - m_nLastOldestEntry <= n100ns)
			return; //Nothing to do

		m_nLastOldestEntry = -1;
	}

	for(std::vector<CachedIcon>::const_iterator it = m_CachedIcons.begin();
		it != m_CachedIcons.end(); ++it)
	{
		const ULONG64 nAge = nNow - it->nTime;

		if (it->nTime < m_nLastOldestEntry)
			m_nLastOldestEntry = it->nTime;

		if (nAge > n100ns)
		{
			const size_t nIndex = it-m_CachedIcons.begin();
			
			if (nIndex == DEFAULTFILEICONINDEX ||
				nIndex == DEFAULTOPENFOLDERICONINDEX ||
				nIndex == DEFAULTCLOSEDFOLDERICONINDEX)
				continue; //Never remove these

			if (m_EntryUsed[nIndex])
			{
				boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

				std::map<int, std::wstring>::iterator it = m_ExtensionsInverse.find(nIndex);

				if (it != m_ExtensionsInverse.end())
				{
					m_Extensions.erase(it->second);
					m_ExtensionsInverse.erase(it);
				}

				m_EntryUsed[nIndex] = false;

				CachedIcon& rCachedIcon = m_CachedIcons[nIndex];

				std::wstring sKey = (boost::wformat(L"%1%:%2%") %rCachedIcon.Location.sPath %rCachedIcon.Location.nIndex).str();

				std::transform(sKey.begin(),sKey.end(),sKey.begin(),tolower);

				rCachedIcon.Location.sPath.clear();
				rCachedIcon.Location.nIndex = 0;

				m_CachedIconsMap.erase(sKey);
			}
		}
	}
}


bool		RefIconList::CachedIcon::IsValid() const
{
	if (Location.sPath.length() == 0)
		return false;

	WIN32_FILE_ATTRIBUTE_DATA data = {0};

	if (!GetFileAttributesEx(Location.sPath.c_str(), GetFileExInfoStandard, &data))
		return false;

	return ((*(ULONG64*)&data.ftLastWriteTime) == nLastModified);
}
/*
*************************************************************************
						IconRegistry
*************************************************************************
*/
IconRegistry::IconRegistry()
{
}


IPtrFSIconRegistry	IconRegistry::GetFSIconRegistry(LPCWSTR sFS, int nVersion, std::wstring& rsFirstPath) const
{
	IPtrFSIconRegistry result;

	if (sFS == NULL || nVersion < 1)
		return IPtrFSIconRegistry();


	WCHAR buffer[MAX_PATH];

	FillLowerCopy(buffer, MAX_PATH, sFS);
	
	size_t nLen = wcslen(buffer);

	wsprintf(&buffer[nLen], L"%i", nVersion);

	std::wstring sKey = buffer;

	{
		boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

		FSIconRegistries::const_iterator it = m_FSIconRegistries.find(sKey);

		if (it != m_FSIconRegistries.end())
		{
			result = it->second.lock();

			if (result.get() != NULL)
			{
				result->AddInstancePath(rsFirstPath);

				return result;
			}
		}
	}

	boost::lock_guard<boost::shared_mutex> rwLock(m_Mutex);

	{
		FSIconRegistries::const_iterator it = m_FSIconRegistries.find(sKey);

		if (it != m_FSIconRegistries.end())
		{
			result = it->second.lock();

			if (result.get() != NULL)
			{
				result->AddInstancePath(rsFirstPath);

				return result;
			}
		}
	}

	result.reset(new FSIconRegistry(sFS, nVersion, const_cast<IconRegistry*>(this), rsFirstPath));
	
	const_cast<IconRegistry*>(this)->m_FSIconRegistries[sKey] = result;

	return result;
}



IPtrRefIconList		IconRegistry::GetRefIconList(ULONG nHeight) const
{
	{
		boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

		for(std::vector<boost::weak_ptr<RefIconList> >::const_iterator it = m_IconLists.begin(); it != m_IconLists.end(); ++it)
		{
			IPtrRefIconList iconList = it->lock();

			if (iconList.get() != NULL && iconList->GetHeight() == nHeight)
				return iconList;
		}
	}
	{
		boost::lock_guard<boost::shared_mutex> rwLock(m_Mutex);

		for(std::vector<boost::weak_ptr<RefIconList> >::const_iterator it = m_IconLists.begin(); it != m_IconLists.end(); )
		{
			IPtrRefIconList iconList = it->lock();

			if (iconList.get() == NULL)
			{
				size_t nIndex = it - m_IconLists.begin();

				const_cast<IconRegistry*>(this)->m_IconLists.erase(it);

				it = m_IconLists.begin() + nIndex;
			}
			else if (iconList->GetHeight() == nHeight)
				return iconList;
			else ++it;
		}

		IPtrRefIconList result(new RefIconList(nHeight));

		const_cast<IconRegistry*>(this)->m_IconLists.push_back(result);

		return result;
	}

}




