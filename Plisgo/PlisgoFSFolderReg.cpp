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





PlisgoFSFolderReg*	PlisgoFSFolderReg::GetSingleton()
{
	static PlisgoFSFolderReg gPlisgoFSFolderReg;

	return &gPlisgoFSFolderReg;
}


PlisgoFSFolderReg::PlisgoFSFolderReg()
{
}


void				PlisgoFSFolderReg::RunRootCacheClean()
{
	bool bCleaningToBeDone = false;

	{
		boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

		for(RootCacheMap::const_iterator it = m_RootCache.begin();
			it != m_RootCache.end(); ++it)
		{
			if(!it->second->IsValid() || !IsRootValidForPath(it->second, it->first))
			{
				bCleaningToBeDone = true;
				break;
			}
		}
	}

	if (bCleaningToBeDone)
	{
		boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

		for(RootCacheMap::const_iterator it = m_RootCache.begin();
			it != m_RootCache.end();)
		{
			if(!it->second->IsValid() || !IsRootValidForPath(it->second, it->first))
				it = m_RootCache.erase(it);
			else
				++it;
		}
	}
}


bool	PlisgoFSFolderReg::IsRootValidForPath(IPtrPlisgoFSRoot root, const std::wstring& rsPath) const
{
	if (root.get() == NULL)
		return false;

	const std::wstring& rsRootPath = root->GetPath();

	if (rsRootPath.length() == rsPath.length()+1) //+1 because of slash
		return true;

	if (rsRootPath.length() > rsPath.length())
		return false;

	std::wstring sPath = rsRootPath;

	sPath += L".plisgofs\\.shellinfo\\";

	size_t nSlash = rsPath.rfind(L'\\');

	if (nSlash == rsRootPath.length()-1)
		return true;
	
	assert(nSlash > rsRootPath.length());

	sPath.append(rsPath.begin() + rsRootPath.length(), rsPath.begin()+nSlash);

	return (GetFileAttributes(sPath.c_str()) != INVALID_FILE_ATTRIBUTES);
}


IPtrPlisgoFSRoot	PlisgoFSFolderReg::GetPlisgoFSRoot(LPCWSTR sPathUnprocessed) const
{
	static ULONG nCleanCountDown = 20; //Not concurrent cache safe, but doesn't matter

	if (--nCleanCountDown == 0)
	{
		const_cast<PlisgoFSFolderReg*>(this)->RunRootCacheClean();
		nCleanCountDown = 20;
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

	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	size_t nSlash = sPath.length();

	std::wstring sSection = sPath;

	while(nSlash != -1)
	{
		sSection.resize(nSlash);

		RootCacheMap::const_iterator it = m_RootCache.find(sSection);

		if (it != m_RootCache.end())
			if (it->second->IsValid() && IsRootValidForPath(it->second, sPath))
			{
				if (sSection.length() != sPath.length())
				{
					IPtrPlisgoFSRoot root = it->second;

					boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

					const_cast<PlisgoFSFolderReg*>(this)->m_RootCache[sPath] = root;

					return root;
				}
				else return it->second;
			}
				
		const DWORD nAttr = GetFileAttributes((sSection + L"\\.plisgofs").c_str());
		
		if (nAttr != INVALID_FILE_ATTRIBUTES && (nAttr & FILE_ATTRIBUTE_DIRECTORY))
		{
			boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

			return const_cast<PlisgoFSFolderReg*>(this)->GetPlisgoFSRoot_Locked(sPath, sSection);
		}

		nSlash = sPath.rfind(L'\\', nSlash-1);
	}

	return IPtrPlisgoFSRoot();
}


IPtrPlisgoFSRoot	PlisgoFSFolderReg::GetPlisgoFSRoot_Locked(const std::wstring& rsPath, const std::wstring& rsSection)
{
	RootCacheMap::const_iterator it = m_RootCache.find(rsSection);

	if (it != m_RootCache.end())
		if (it->second->IsValid() && IsRootValidForPath(it->second, rsPath))
		{
			m_RootCache[rsPath] = it->second;

			return it->second;
		}

	IPtrPlisgoFSRoot root = const_cast<PlisgoFSFolderReg*>(this)->CreateRoot(rsSection);

	if (root.get() != NULL && IsRootValidForPath(root, rsPath))
	{
		m_RootCache[rsPath] = root;
		m_RootCache[rsSection] = root;
	}
	else root.reset();
		
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
		m_RootCache[rsRoot] = result;
	else
		result.reset();

	return result;
}
