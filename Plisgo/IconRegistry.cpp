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

static void GetPlisgoFileLock(std::string& rsFile)
{
	char sTemp[MAX_PATH];

	GetTempPathA(MAX_PATH, sTemp);

	strcat_s(sTemp, MAX_PATH, "plisgo");

	if (GetFileAttributesA(sTemp) == INVALID_FILE_ATTRIBUTES)
		CreateDirectoryA(sTemp, NULL);

	rsFile.assign(sTemp);
	rsFile += "\\lock";

	if (GetFileAttributesA(rsFile.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		HANDLE hFile = CreateFileA(rsFile.c_str(), FILE_GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);

		if (hFile != NULL)
			CloseHandle(hFile);
	}
}



static bool OldAndShouldBeDeleted(const std::string& rsInfoFile)
{
	bool bDelete = false;

	HANDLE hFile = CreateFileA(rsInfoFile.c_str() , FILE_GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile != NULL && hFile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER nSize = {0};

		GetFileSizeEx(hFile, &nSize);

		if (nSize.HighPart == 0 && nSize.LowPart != 0)
		{
			DWORD nDataSize = nSize.LowPart - sizeof(ULONG64)*2;

			BYTE* sData = (BYTE*)malloc(nDataSize);
		
			if (sData != NULL)
			{
				DWORD nRead = 0;
				ULONG64 nBaseTime;
				ULONG64 nOverTime;
				ULONG32 nLength = 0;

				if (ReadFile(hFile, &nBaseTime, sizeof(ULONG64), &nRead, NULL) && nRead == sizeof(ULONG64) &&
					ReadFile(hFile, &nOverTime, sizeof(ULONG64), &nRead, NULL) && nRead == sizeof(ULONG64) &&
					ReadFile(hFile, &nLength, sizeof(ULONG32), &nRead, NULL) && nRead == sizeof(ULONG32) &&
					ReadFile(hFile, sData, nDataSize, &nRead, NULL)) 
				{
					WCHAR* sBase = (WCHAR*)sData;

					if (nRead > ((nLength * sizeof(WCHAR)) + (sizeof(ULONG32)*2)))
					{
						WCHAR* sOver = (WCHAR*)&((ULONG32*)&sBase[nLength])[2]; //Skip Base Index and Over Length

						WIN32_FILE_ATTRIBUTE_DATA baseInfo = {0};
						WIN32_FILE_ATTRIBUTE_DATA overInfo = {0};

						if (GetFileAttributesEx(sBase, GetFileExInfoStandard, &baseInfo) &&
							GetFileAttributesEx(sOver, GetFileExInfoStandard, &overInfo))
						{
							if (nBaseTime != *(ULONG64*)&baseInfo.ftLastWriteTime ||
								nOverTime != *(ULONG64*)&overInfo.ftLastWriteTime)
								bDelete = true;	
						}
						else bDelete = true;						
					}
				}

				free(sData);
			}
		}

		CloseHandle(hFile);
	}

	return bDelete;
}


static volatile LONG gPlisgoTempFileCleanUpThreadCheck = 0;

static ULONG64	gnPlisgoTempFileCleanUpTheadCheckTime = 0;

static HANDLE ghPlisgoTempFileCleanUpThread = NULL;


static DWORD WINAPI CleanUpOldPlisgoTempFilesCB( LPVOID  )
{
	char sTemp[MAX_PATH];

	GetTempPathA(MAX_PATH, sTemp);

	strcat_s(sTemp, MAX_PATH, "plisgo");

	if (GetFileAttributesA(sTemp) == INVALID_FILE_ATTRIBUTES)
		CreateDirectoryA(sTemp, NULL);

	strcat_s(sTemp, MAX_PATH, "\\");

	std::string sLockFile;

	GetPlisgoFileLock(sLockFile);

	boost::interprocess::file_lock fileLock(sLockFile.c_str());

	WIN32_FIND_DATAA findData;

	HANDLE hFind = FindFirstFileA((std::string(sTemp) += "*.info").c_str(), &findData);

	if (hFind != NULL && hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			std::string sInfoFile(sTemp);
			sInfoFile+= findData.cFileName;

			boost::interprocess::scoped_lock<boost::interprocess::file_lock> lock(fileLock);

			if (OldAndShouldBeDeleted(sInfoFile))
			{
				DeleteFileA(sInfoFile.c_str());
				size_t nDot = sInfoFile.rfind(L'.');
				if (nDot != -1)
					DeleteFileA((sInfoFile.substr(0, nDot) += ".ico").c_str());
			}
		}
		while(FindNextFileA(hFind, &findData) != 0);

		FindClose(hFind);
	}

	//Spin until got lock
	while(InterlockedCompareExchange(&gPlisgoTempFileCleanUpThreadCheck, 1, 0) != 0);

	HANDLE hHandle = ghPlisgoTempFileCleanUpThread;
	ghPlisgoTempFileCleanUpThread = NULL;

	InterlockedExchange(&gPlisgoTempFileCleanUpThreadCheck, 0); //Release lock

	CloseHandle(hHandle);

	return 0;
}


static void RunTempFileCheanUp()
{
	if (InterlockedCompareExchange(&gPlisgoTempFileCleanUpThreadCheck, 1, 0) == 0) //Try lock
	{
		ULONG64 nNow = 0;

		GetSystemTimeAsFileTime((FILETIME*)&nNow);

		if (nNow-gnPlisgoTempFileCleanUpTheadCheckTime > NTMINUTE*15)
		{
			if (ghPlisgoTempFileCleanUpThread == NULL)
			{
				gnPlisgoTempFileCleanUpTheadCheckTime = nNow;
				ghPlisgoTempFileCleanUpThread = CreateThread(NULL, 0, CleanUpOldPlisgoTempFilesCB, NULL, 0, NULL);
			}
		}

		InterlockedExchange(&gPlisgoTempFileCleanUpThreadCheck, 0); //release lock
	}
}



static bool WriteIconLocationToInfoFile(HANDLE hFile, const IconLocation& rIconLocation)
{
	DWORD nWritten = 0;
	ULONG32 nLength = (ULONG32)rIconLocation.sPath.length()+1;

	WriteFile(hFile, &nLength, sizeof(ULONG32), &nWritten, NULL);

	if (nWritten != sizeof(ULONG32))
		return false;

	WriteFile(hFile, rIconLocation.sPath.c_str(), nLength*sizeof(WCHAR), &nWritten, NULL);
	
	if (nWritten != nLength*sizeof(WCHAR))
		return false;

	nLength = (ULONG32)rIconLocation.nIndex;
	WriteFile(hFile, &nLength, sizeof(ULONG32), &nWritten, NULL);

	return (nWritten == sizeof(ULONG32));
}


static bool	WriteInfoFile(LPCWSTR sFile, const IconLocation& rFirst, ULONG64 nFirstTime, const IconLocation& rSecond, ULONG64 sSecondTime)
{
	HANDLE hFile = CreateFileW(sFile, FILE_WRITE_DATA, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, NULL);

	if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
		return false;

	DWORD nWritten = 0;

	bool bResult = false;

	if (WriteFile(hFile, &nFirstTime, sizeof(ULONG64), &nWritten, NULL) && nWritten == sizeof(ULONG64) &&
		WriteFile(hFile, &sSecondTime, sizeof(ULONG64), &nWritten, NULL) && nWritten == sizeof(ULONG64))
	{
		bResult = true;

		//This bit doesn't matter as much, it's to be used for book keeping
		if (WriteIconLocationToInfoFile(hFile, rFirst))
			WriteIconLocationToInfoFile(hFile, rSecond);
	}

	CloseHandle(hFile);

	return bResult;
}


static bool IsUpToDateInfoFile(const std::wstring& rsTxtFile, ULONG64 nBaseTime, ULONG64 nOverTime)
{
	bool bResult = false;

	HANDLE hFile = CreateFileW(rsTxtFile.c_str(), FILE_READ_DATA, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile != NULL && hFile != INVALID_HANDLE_VALUE)
	{
		OVERLAPPED overlapped = {0};

		ULONG64 nFileBaseTime;
		ULONG64 nFileOverTime;

		DWORD nRead = 0;

		if (ReadFile(hFile, &nFileBaseTime, sizeof(ULONG64), &nRead, NULL) && nRead == sizeof(ULONG64) &&
			ReadFile(hFile, &nFileOverTime, sizeof(ULONG64), &nRead, NULL) && nRead == sizeof(ULONG64) )
		{
			bResult = (nBaseTime == nFileBaseTime && nOverTime == nFileOverTime);
		}

		CloseHandle(hFile);
	}

	return bResult;
}


ULONG64	IconLocation::GetHash() const
{
	ULONG64 nResult = 0;

	//No it's not thread safe

	for(std::wstring::const_iterator it = sPath.begin();
		it != sPath.end(); ++it)
	{
		AddToPreHash64(nResult, tolower(*it));
	}

	AddToPreHash64(nResult, nIndex);

	return HashFromPreHash(nResult);
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
		
			VersionedImageList& rVersionedImageList = m_ImageLists[nIndex];

			const size_t nVersionIndex = rVersionedImageList.size();

			rVersionedImageList.resize(rVersionedImageList.size()+1);

			ImageListVersion& rEntry = rVersionedImageList[nVersionIndex];

			rEntry.sExt		= wcsrchr(findData.cFileName, L'.');
			rEntry.nHeight	= nHeight;
		}
		while(FindNextFileW(hFind, &findData) != 0);

		FindClose(hFind);
	}
}


FSIconRegistry::~FSIconRegistry()
{

}


bool	FSIconRegistry::GetBestImageList(std::wstring& rsImageListFile, UINT nList, UINT nHeight) const
{
	if (nList >= m_ImageLists.size())
		return false;

	const VersionedImageList& rVersionedList = m_ImageLists[nList];

	if (rVersionedList.size() == 0)
		return false;

	std::wstring sInstancePath;

	if (!GetInstancePath(sInstancePath))
		return false;

	VersionedImageList::const_iterator it		= rVersionedList.begin();
	VersionedImageList::const_iterator bestIt	= it;

	if (it == rVersionedList.end())
		return false;
	
	while(it != rVersionedList.end())
	{
		if (it->nHeight == nHeight)
		{
			bestIt = it;
			break;
		}
		else if ( it->nHeight > bestIt->nHeight && it->nHeight <= nHeight)
		{
			bestIt = it;
		}

		it++;
	}	

	rsImageListFile = (boost::wformat(L"%1%.icons_%2%_%3%%4%") %sInstancePath %nList %bestIt->nHeight %bestIt->sExt).str();

	return true;
}


void	FSIconRegistry::AddInstancePath(std::wstring sRootPath)
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


void	FSIconRegistry::RemoveInstancePath(std::wstring sRootPath)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	std::transform(sRootPath.begin(),sRootPath.end(),sRootPath.begin(), tolower);

	if (sRootPath.length() && *(sRootPath.end()-1) != L'\\')
		sRootPath += L'\\';

	//Trim dead paths
	for(WStringList::iterator it = m_Paths.begin(); it != m_Paths.end();)
		if (GetFileAttributes((*it).c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			if (*it == sRootPath)
				it = m_Paths.erase(it);
			else
				++it;
		}
		else it = m_Paths.erase(it);
}


bool	FSIconRegistry::GetInstancePath(std::wstring& rsResult) const
{
	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	for(WStringList::const_iterator it = m_Paths.begin(); it != m_Paths.end();)
	{
		//Trim dead paths
		if (GetFileAttributes((*it).c_str()) == INVALID_FILE_ATTRIBUTES)
		{
			{
				boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

				for(WStringList::const_iterator it2 = m_Paths.begin(); it2 != m_Paths.end();)
					if (GetFileAttributes((*it2).c_str()) == INVALID_FILE_ATTRIBUTES)
						it2 = const_cast<FSIconRegistry*>(this)->m_Paths.erase(it2);
					else
						++it2;
			}

			it = m_Paths.begin();
		}
		else
		{
			rsResult = *it;
			return true;
		}
	}

	return false;
}


bool	FSIconRegistry::ReadIconLocation(IconLocation& rIconLocation, const std::wstring& rsPligoFile, const ULONG nHeight)
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


bool	FSIconRegistry::GetIconLocation(IconLocation& rIconLocation, UINT nList, UINT nIndex, const ULONG nHeight) const
{
	if (!GetBestImageList(rIconLocation.sPath, nList, nHeight))
		return false;

	rIconLocation.nIndex = nIndex;

	return true;
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


bool	RefIconList::GetFileIconLocation(IconLocation& rIconLocation, const std::wstring& rsPath) const
{
	if (ExtractOwnIconInfoOfFile(rIconLocation.sPath, rIconLocation.nIndex, rsPath))
	{
		//Has it's own icon

		int nTemp;

		if (GetIconLocationIndex(nTemp, rIconLocation))
			return true;

		//But it can't be loaded........ use the default
	}

	std::wstring sKey;

	GetLowerCaseExtension(sKey, rsPath);

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


bool	RefIconList::AddEntry_RW(int& rnIndex, HICON hIcon)
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


int		RefIconList::GetFreeSlot()
{
	for(std::vector<bool>::iterator it = m_EntryUsed.begin();
		it != m_EntryUsed.end(); ++it)
	{
		if (*it == false)
		{
			*it = true;
			return (int)(it-m_EntryUsed.begin());
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


bool	RefIconList::GetFolderIconLocation(IconLocation& rIconLocation, bool bOpen) const
{
	const CachedIcon& rCachedIcon = m_CachedIcons[(bOpen)?DEFAULTOPENFOLDERICONINDEX:DEFAULTCLOSEDFOLDERICONINDEX];

	rIconLocation = rCachedIcon.Location;

	return true;
}


bool	RefIconList::GetIconLocation(IconLocation& rIconLocation, const int nIndex) const
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


bool	RefIconList::GetIcon(HICON& rhResult, const int nIndex) const
{
	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	assert(nIndex < (int)m_CachedIcons.size());

	GetSystemTimeAsFileTime((FILETIME*)&m_CachedIcons[nIndex].nTime);

	return (m_ImageList->GetIcon(nIndex, ILD_TRANSPARENT, &rhResult) == S_OK);
}


bool	RefIconList::GetIcon(HICON& rhResult, const IconLocation& rIconLocation) const
{
	int nIndex = 0;

	if (!GetIconLocationIndex(nIndex, rIconLocation))
		return false;

	return GetIcon(rhResult, nIndex);
}


bool	RefIconList::GetIconLocationIndex(int& rnIndex, const IconLocation& rIconLocation) const
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


bool	RefIconList::GetIconLocationIndex_RW(int& rnIndex, const IconLocation& rIconLocation, const std::wstring& rsKey)
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

	hResult = EnsureIconSizeResolution(hResult, m_nHeight);

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


void	RefIconList::RemoveOlderThan(ULONG64 n100ns)
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

				std::map<int, std::wstring>::iterator it = m_ExtensionsInverse.find((int)nIndex);

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


HICON	RefIconList::CreateOverlaidIcon(const IconLocation& rFirst,	const IconLocation& rSecond)
{
	HICON hBase = NULL;

	if (!GetIcon(hBase, rFirst) || hBase == NULL)
		return NULL;
	
	HICON hSecond = GetSpecificIcon(rSecond.sPath, rSecond.nIndex, m_nHeight);

	if (hSecond == NULL)
	{
		DestroyIcon(hBase);

		return NULL;
	}

	POINT basePos = {0};
	POINT overlayPos = {m_nHeight,m_nHeight};

	ICONINFO iconinfo;

	if (GetIconInfo(hSecond, &iconinfo))
	{
		BITMAP bm;

		GetObject(iconinfo.hbmColor, sizeof(bm), &bm);

		overlayPos.x -= bm.bmHeight;
		overlayPos.y -= bm.bmHeight;

		overlayPos.x = max(overlayPos.x, 0);
		overlayPos.y = max(overlayPos.y, 0);

		DeleteObject(iconinfo.hbmColor);
		DeleteObject(iconinfo.hbmMask);
	}

	HICON hResult = BurnTogether(hBase, basePos, hSecond, overlayPos, m_nHeight, (m_nHeight > 48));

	DestroyIcon(hSecond);
	DestroyIcon(hBase);

	return hResult;
}




bool	RefIconList::MakeOverlaid(	IconLocation&		rDst,
									const IconLocation& rFirst,	
									const IconLocation& rSecond)
{
	if (!rFirst.IsValid() || !rSecond.IsValid())
		return false;

	WIN32_FILE_ATTRIBUTE_DATA baseInfo = {0};
	WIN32_FILE_ATTRIBUTE_DATA overInfo = {0};

	if (!GetFileAttributesEx(rFirst.sPath.c_str(), GetFileExInfoStandard, &baseInfo) ||
		!GetFileAttributesEx(rSecond.sPath.c_str(), GetFileExInfoStandard, &overInfo))
	{
		return false;
	}

	const ULONG64 nBaseHash = rFirst.GetHash();
	const ULONG64 nOverHash = rSecond.GetHash();
	const ULONG64 nBaseTime = *(ULONG64*)&baseInfo.ftLastWriteTime;
	const ULONG64 nOverTime = *(ULONG64*)&overInfo.ftLastWriteTime;

	WCHAR sTemp[MAX_PATH];

	GetTempPath(MAX_PATH, sTemp);

	const std::wstring sPlisgoTemp = (boost::wformat(L"%1%plisgo") %sTemp).str();	
	const std::wstring sFile((boost::wformat(L"%1%\\%2%_%3%_%4%") %sPlisgoTemp %nBaseHash %nOverHash %m_nHeight).str());


	rDst.nIndex = 0;
	rDst.sPath = sFile;
	rDst.sPath += L".ico";

	std::wstring sTxtFile = sFile;
	sTxtFile+= L".info";

	std::string sLockFile;

	GetPlisgoFileLock(sLockFile);

	boost::interprocess::file_lock fileLock(sLockFile.c_str());

	{
		boost::interprocess::sharable_lock<boost::interprocess::file_lock> lock(fileLock);

		if (IsUpToDateInfoFile(sTxtFile, nBaseTime, nOverTime))
			if (GetFileAttributes(rDst.sPath.c_str()) != INVALID_FILE_ATTRIBUTES)
				return true;
	}


	{
		boost::interprocess::scoped_lock<boost::interprocess::file_lock> lock(fileLock);

		if (IsUpToDateInfoFile(sTxtFile, nBaseTime, nOverTime))
			if (GetFileAttributes(rDst.sPath.c_str()) != INVALID_FILE_ATTRIBUTES)
				return true;

		bool bResult = false;

		HICON hResult = CreateOverlaidIcon(rFirst, rSecond);
			
		if (hResult != NULL)
		{
			if (WriteToIconFile(rDst.sPath.c_str(), hResult))
			{
				WriteInfoFile(sTxtFile.c_str(), rFirst, nBaseTime, rSecond, nOverTime);

				bResult = true;
			}

			DestroyIcon(hResult);
		}
	
		return bResult;
	}
}


bool	RefIconList::CachedIcon::IsValid() const
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
	RunTempFileCheanUp();
}


IPtrFSIconRegistry	IconRegistry::GetFSIconRegistry(LPCWSTR sFS, int nVersion, const std::wstring& rsInstancePath) const
{
	RunTempFileCheanUp();

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
			result = it->second;
			
			result->AddInstancePath(rsInstancePath);

			return result;
		}
	}

	boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

	{
		FSIconRegistries::const_iterator it = m_FSIconRegistries.find(sKey);

		if (it != m_FSIconRegistries.end())
		{
			result = it->second;
			
			result->AddInstancePath(rsInstancePath);

			return result;
		}
	}

	result.reset(new FSIconRegistry(sFS, nVersion, const_cast<IconRegistry*>(this), rsInstancePath));
	
	const_cast<IconRegistry*>(this)->m_FSIconRegistries[sKey] = result;

	return result;
}


void				IconRegistry::ReleaseFSIconRegistry(IPtrFSIconRegistry& rFSIconRegistry,
														const std::wstring& rsInstancePath)
{
	assert(rFSIconRegistry.get() != NULL);

	std::wstring sKey = (boost::wformat(L"%1%:%2%") %rFSIconRegistry->GetFSName() %rFSIconRegistry->GetFSVersion()).str();

	std::transform(sKey.begin(),sKey.end(),sKey.begin(), tolower);

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	rFSIconRegistry->RemoveInstancePath(rsInstancePath);

	if (m_FSIconRegistries.size() && (m_FSIconRegistries.find(sKey) != m_FSIconRegistries.end()))
	{
		std::wstring sCurrentInstacePath;

		if (!rFSIconRegistry->GetInstancePath(sCurrentInstacePath))
		{
			//No instance paths left, remove from map

			m_FSIconRegistries.erase(sKey);
		}
	}
	else
	{
		//It has already been removed. TODO
	}
}


IPtrRefIconList		IconRegistry::GetRefIconList(ULONG nHeight) const
{
	RunTempFileCheanUp();

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




