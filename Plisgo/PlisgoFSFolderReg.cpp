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
#include "PlisgoFSFolderReg.h"



static volatile LONG gnCleaningThreadCheck = 0;

static ULONG64	gnCleaningTheadCheckTime = 0;

static HANDLE	ghCleaningThread = NULL;


static DWORD WINAPI ClearningThreadCB(LPVOID)
{
	static IUnknown* pExplorer = NULL;

	SHGetInstanceExplorer(&pExplorer);

	PlisgoFSFolderReg::GetSingleton()->RunRootCacheClean();

	//Spin until got lock
	while(InterlockedCompareExchange(&gnCleaningThreadCheck, 1, 0) != 0);

	HANDLE hHandle = ghCleaningThread;
	ghCleaningThread = NULL;

	InterlockedExchange(&gnCleaningThreadCheck, 0); //Release lock

	CloseHandle(hHandle);

	if (pExplorer != NULL)
		pExplorer->Release();

	return 0;
}




PlisgoFSFolderReg*	PlisgoFSFolderReg::GetSingleton()
{
	static PlisgoFSFolderReg gPlisgoFSFolderReg;

	if (InterlockedCompareExchange(&gnCleaningThreadCheck, 1, 0) == 0) //Try lock
	{
		ULONG64 nNow = 0;

		GetSystemTimeAsFileTime((FILETIME*)&nNow);

		if (nNow-gnCleaningTheadCheckTime > NTMINUTE*5)
		{
			if (ghCleaningThread == NULL)
			{
				gnCleaningTheadCheckTime = nNow;
				ghCleaningThread = CreateThread(NULL, 0, ClearningThreadCB, NULL, 0, NULL);
			}
		}

		InterlockedExchange(&gnCleaningThreadCheck, 0); //release lock
	}

	return &gPlisgoFSFolderReg;
}


PlisgoFSFolderReg::PlisgoFSFolderReg()
{
}


void				PlisgoFSFolderReg::RunRootCacheClean()
{
	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	bool bCleaningToBeDone = false;

	for(RootCacheMap::const_iterator it = m_RootCache.begin();
		it != m_RootCache.end(); ++it)
	{
		if( GetFileAttributes(it->second->GetPath().c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			if( GetFileAttributes((it->second->GetPath() + L".plisgofs").c_str()) == INVALID_FILE_ATTRIBUTES)
			{
				bCleaningToBeDone = true;
				break;
			}
		}
		else
		{
			bCleaningToBeDone = true;
			break;
		}
	}

	if (bCleaningToBeDone)
	{
		boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

		bCleaningToBeDone = false;

		for(RootCacheMap::const_iterator it = m_RootCache.begin();
			it != m_RootCache.end();)
		{
			if( GetFileAttributes(it->second->GetPath().c_str()) != INVALID_FILE_ATTRIBUTES)
			{
				if( GetFileAttributes((it->second->GetPath() + L".plisgofs").c_str()) == INVALID_FILE_ATTRIBUTES)
					bCleaningToBeDone = true;
			}
			else bCleaningToBeDone = true;

			if (bCleaningToBeDone)
			{
				it = m_RootCache.erase(it);
				bCleaningToBeDone = false;
			}
			else ++it;
		}
	}
}


IPtrPlisgoFSRoot	PlisgoFSFolderReg::ReturnValidRoot(IPtrPlisgoFSRoot root, const std::wstring& rsPath) const
{
	if (root.get() == NULL)
		return root;

	const std::wstring& rsRootPath = root->GetPath();

	if (rsRootPath.length() == rsPath.length()+1) //+1 because of slash
		return root;

	if (rsRootPath.length() > rsPath.length())
		return IPtrPlisgoFSRoot(); //what the crap?

	std::wstring sPath = rsRootPath;

	sPath += L".plisgofs\\.shellinfo\\";
	sPath.append(rsPath.begin() + rsRootPath.length(), rsPath.end());

	if (GetFileAttributes(sPath.c_str()) == INVALID_FILE_ATTRIBUTES)
		return IPtrPlisgoFSRoot();

	return root;
}


IPtrPlisgoFSRoot	PlisgoFSFolderReg::GetPlisgoFSRoot(LPCWSTR sPathUnprocessed) const
{
	assert(sPathUnprocessed != NULL);

	std::wstring sPath;

	{
		const size_t nLen = wcslen(sPathUnprocessed);

		if (nLen == 0)
			return IPtrPlisgoFSRoot();

		sPath.assign(nLen, L' ');

		std::transform(sPathUnprocessed, sPathUnprocessed+nLen, sPath.begin(), PrePathCharacter);
	}

	
	boost::trim_right_if(sPath, boost::is_any_of(L"\\"));

	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	size_t nSlash = sPath.length();

	std::wstring sSection = sPath;

	while(nSlash != -1)
	{
		sSection.resize(nSlash);

		const DWORD nAttr = GetFileAttributes((sSection + L"\\.plisgofs").c_str());

		RootCacheMap::const_iterator it = m_RootCache.find(sSection);

		if (it != m_RootCache.end())
		{
			if (nAttr == INVALID_FILE_ATTRIBUTES || !(nAttr & FILE_ATTRIBUTE_DIRECTORY))
			{
				boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

				it = m_RootCache.find(sSection);

				if (it != m_RootCache.end())
					const_cast<PlisgoFSFolderReg*>(this)->m_RootCache.erase(it);
			}
			else return ReturnValidRoot(it->second, sPath);
		}
		else if (nAttr != INVALID_FILE_ATTRIBUTES && (nAttr & FILE_ATTRIBUTE_DIRECTORY))
		{
			boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

			it = m_RootCache.find(sSection);

			if (it != m_RootCache.end())
				return ReturnValidRoot(it->second, sPath);

			IPtrPlisgoFSRoot root = const_cast<PlisgoFSFolderReg*>(this)->CreateRoot(sSection);

			return ReturnValidRoot(root, sPath);
		}

		nSlash = sPath.rfind(L'\\', nSlash-1);
	}

	return IPtrPlisgoFSRoot();
}


IPtrPlisgoFSRoot	PlisgoFSFolderReg::CreateRoot(const std::wstring& rsRoot)
{
	IPtrPlisgoFSRoot result;

	PlisgoFSRoot* pNewRoot = new PlisgoFSRoot(rsRoot, &m_IconRegistry);

	if (pNewRoot->GetFSName()[0] != L'\0')
	{
		result.reset(pNewRoot);

		m_RootCache[rsRoot] = result;
	}
	else delete pNewRoot;

	return result;
}
