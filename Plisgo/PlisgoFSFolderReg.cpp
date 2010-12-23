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

#define MAXROOTCHECKGAP		NTSECOND*2
#define MAXPATHROOTCHECKGAP	NTSECOND*2



PlisgoFSFolderReg*	PlisgoFSFolderReg::GetSingleton()
{
	static PlisgoFSFolderReg gPlisgoFSFolderReg;

	return &gPlisgoFSFolderReg;
}


PlisgoFSFolderReg::PlisgoFSFolderReg()
{
	m_pOldestRootHolder = NULL;
	m_pNewestRootHolder = NULL;
}


void				PlisgoFSFolderReg::RunRootCacheClean()
{
	ULONG64 nNow;

	GetSystemTimeAsFileTime((FILETIME*)&nNow);

	if (m_pOldestRootHolder != NULL && (nNow - m_pOldestRootHolder->nLastCheckTime) > MAXROOTCHECKGAP)
	{
		if (m_pOldestRootHolder->root->IsValid())
		{
			//You pass, move to the back
			
			m_pOldestRootHolder->nLastCheckTime = nNow;

			RootHolder* pEntry = m_pOldestRootHolder->pNext;

			if (pEntry != NULL)
			{
				m_pOldestRootHolder->pPrev = m_pNewestRootHolder;
				m_pOldestRootHolder->pNext = NULL;
				
				m_pNewestRootHolder->pNext = m_pOldestRootHolder;

				pEntry->pPrev = NULL;

				m_pNewestRootHolder = m_pOldestRootHolder;
				m_pOldestRootHolder = pEntry;
			}
			//else it's alone!
		}
		else
		{
			RootHolder* pOldEntry = m_pOldestRootHolder;

			if (m_pOldestRootHolder != m_pNewestRootHolder)
			{
				RootHolder* pEntry = m_pOldestRootHolder->pNext;

				pEntry->pPrev = NULL;
				m_pOldestRootHolder = pEntry;
			}
			else m_pOldestRootHolder = m_pNewestRootHolder = NULL;

			m_RootHolderPool.destroy(pOldEntry);


			for(RootCacheMap::iterator it = m_RootCache.begin(); it != m_RootCache.end();)
			{
				if (it->second.root.expired())
					it = m_RootCache.erase(it);
				else
					++it;
			}
		}
	}
}


bool	PlisgoFSFolderReg::IsRootValidForPath(const std::wstring& rsRootPath, const std::wstring& rsPath) const
{
	if (rsRootPath.length() == rsPath.length()+1) //+1 because of slash
		return true;

	if (rsRootPath.length() > rsPath.length())
		return false;

	std::wstring sPath = rsRootPath;

	sPath += L".plisgofs\\.shellinfo\\";

	const DWORD nAttr = GetFileAttributes(rsPath.c_str());

	if (nAttr == INVALID_FILE_ATTRIBUTES)
		return false;

	if (nAttr & FILE_ATTRIBUTE_DIRECTORY)
	{
		sPath.append(rsPath.begin() + rsRootPath.length(), rsPath.end());
	}
	else
	{
		size_t nSlash = rsPath.rfind(L'\\');

		if (nSlash == rsRootPath.length()-1)
			return true;
		
		assert(nSlash > rsRootPath.length());

		sPath.append(rsPath.begin() + rsRootPath.length(), rsPath.begin()+nSlash);
	}

	return (GetFileAttributes(sPath.c_str()) != INVALID_FILE_ATTRIBUTES);
	
}


IPtrPlisgoFSRoot	PlisgoFSFolderReg::GetPlisgoFSRoot(LPCWSTR sPathUnprocessed) const
{
	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	ULONG64 nNow;

	GetSystemTimeAsFileTime((FILETIME*)&nNow);

	if (m_pOldestRootHolder != NULL && (nNow - m_pOldestRootHolder->nLastCheckTime) > NTSECOND*30)
	{
		boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

		const_cast<PlisgoFSFolderReg*>(this)->RunRootCacheClean();
	}

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

	size_t nSlash = sPath.length();

	std::wstring sSection = sPath;

	GetSystemTimeAsFileTime((FILETIME*)&nNow); //Get again, as above might have waited for write lock

	while(nSlash != -1)
	{
		sSection.resize(nSlash);

		RootCacheMap::const_iterator it = m_RootCache.find(sSection);

		if (it != m_RootCache.end())
		{
			IPtrPlisgoFSRoot root = it->second.root.lock();

			if (root.get() != NULL)
			{
				if (sSection.length() == sPath.length()) //Shortcut
					if (nNow - it->second.nLastCheckTime < MAXPATHROOTCHECKGAP)
						return root;

				if (root->IsValid() && IsRootValidForPath(root->GetPath(), sPath))
				{
					boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

					PathRootCacheEntry& rEntry = const_cast<PlisgoFSFolderReg*>(this)->m_RootCache[sPath];

					rEntry.root = root;

					GetSystemTimeAsFileTime((FILETIME*)&rEntry.nLastCheckTime); //Again, check it, there is a write lock, thus wait

					return root;
				}
			}
		}
				
		const DWORD nAttr = GetFileAttributes((sSection + L"\\.plisgofs").c_str());
		
		if (nAttr != INVALID_FILE_ATTRIBUTES && (nAttr & FILE_ATTRIBUTE_DIRECTORY))
		{
			if (IsRootValidForPath(sSection + L"\\", sPath))
			{
				boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

				return const_cast<PlisgoFSFolderReg*>(this)->GetPlisgoFSRoot_Locked(sPath, sSection);
			}
		}

		nSlash = sPath.rfind(L'\\', nSlash-1);
	}

	return IPtrPlisgoFSRoot();
}


IPtrPlisgoFSRoot	PlisgoFSFolderReg::GetPlisgoFSRoot_Locked(const std::wstring& rsPath, const std::wstring& rsSection)
{
	// we know IsRootValidForPath is true for the rsPath+rsSection or we won't be here.
	RootCacheMap::const_iterator it = m_RootCache.find(rsSection);

	ULONG64 nNow;

	GetSystemTimeAsFileTime((FILETIME*)&nNow);

	IPtrPlisgoFSRoot root;

	if (it != m_RootCache.end())
	{
		root = it->second.root.lock();

		if (root.get() != NULL)
		{
			if (rsSection.length() == rsPath.length()) //Shortcut
				if (nNow - it->second.nLastCheckTime < MAXPATHROOTCHECKGAP)
					return root;

			if (root->IsValid() && IsRootValidForPath(root->GetPath(), rsPath))
			{
				PathRootCacheEntry& rEntry = m_RootCache[rsPath];

				rEntry.root = root;
				rEntry.nLastCheckTime = nNow;

				return root;
			}
		}
	}

	root = CreateRoot(rsSection);

	if (root.get() != NULL)
	{
		PathRootCacheEntry& rEntry = m_RootCache[rsPath];

		rEntry.root = root;
		rEntry.nLastCheckTime = nNow;

		PathRootCacheEntry& rRootEntry = m_RootCache[rsSection];

		rRootEntry.root = root;
		rRootEntry.nLastCheckTime = nNow;
	}
		
	return root;
}


IPtrPlisgoFSRoot	PlisgoFSFolderReg::CreateRoot(const std::wstring& rsRoot)
{
	IPtrPlisgoFSRoot result;

	PlisgoFSRoot* pNewRoot = new PlisgoFSRoot;

	assert(pNewRoot != NULL);

	result.reset(pNewRoot);

	pNewRoot->Init(rsRoot);

	if (pNewRoot->GetFSName()[0] != L'\0')
	{
		RootHolder* pNewHolder = m_RootHolderPool.construct();

		pNewHolder->root = result;
		GetSystemTimeAsFileTime((FILETIME*)&pNewHolder->nLastCheckTime);

		if (m_pOldestRootHolder != NULL)
		{
			assert(m_pNewestRootHolder != NULL);

			m_pNewestRootHolder->pNext = pNewHolder;
			pNewHolder->pPrev = m_pNewestRootHolder;
			pNewHolder->pNext = NULL;

			m_pNewestRootHolder = pNewHolder;
		}
		else
		{
			assert(m_pNewestRootHolder == NULL);

			pNewHolder->pPrev = NULL;
			pNewHolder->pNext = NULL;

			m_pNewestRootHolder = m_pOldestRootHolder = pNewHolder;
		}
	}
	else result.reset();

	return result;
}
