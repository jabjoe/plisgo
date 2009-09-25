/*
    Copyright 2009 Eurocom Entertainment Software Limited

    This file is part of Plisgo.

    Eurocom Entertainment Software Limited, hereby disclaims all
    copyright interest in �Plisgo� written by Joe Burmeister.

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


static PlisgoFSFolderReg* gpPlisgoFSFolderReg = NULL;


void				PlisgoFSFolderReg::Init()
{
	assert(gpPlisgoFSFolderReg == NULL);

	gpPlisgoFSFolderReg = new PlisgoFSFolderReg();
}


PlisgoFSFolderReg*	PlisgoFSFolderReg::GetSingleton()
{
	return gpPlisgoFSFolderReg;
}


void				PlisgoFSFolderReg::Shutdown()
{
	assert(gpPlisgoFSFolderReg != NULL);

	delete gpPlisgoFSFolderReg;
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

		if (GetFileAttributes(sPathUnprocessed) == INVALID_FILE_ATTRIBUTES)
			return IPtrPlisgoFSRoot();

		sPath.assign(nLen, L' ');

		std::transform(sPathUnprocessed, sPathUnprocessed+nLen, sPath.begin(), PrePathCharacter);
	}

	//Remove trailing slash for consistancy.
	if (sPath[sPath.length()-1] == L'\\')
		sPath.resize(sPath.length()-1);


	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	RootCacheMap::const_iterator it = m_RootCache.find(sPath);

	if (it != m_RootCache.end())
		return ReturnValidRoot(it->second, sPath);

	size_t nSlash = sPath.length();

	while(nSlash != -1)
	{
		std::wstring sSection = sPath.substr(0, nSlash);

		it = m_RootCache.find(sSection);

		if (it != m_RootCache.end())
			return ReturnValidRoot(it->second, sPath);

		nSlash = sPath.rfind(L'\\', nSlash-1);
	}


	boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

	it = m_RootCache.find(sPath);

	if (it != m_RootCache.end())
		return ReturnValidRoot(it->second, sPath);

	nSlash = sPath.length();

	while(nSlash != -1)
	{
		std::wstring sSection = sPath.substr(0, nSlash);

		it = m_RootCache.find(sSection);

		if (it != m_RootCache.end())
			return ReturnValidRoot(it->second, sPath);

		const DWORD nAttr = GetFileAttributes((sSection + L"\\.plisgofs\\").c_str());

		if (nAttr != INVALID_FILE_ATTRIBUTES && (nAttr & FILE_ATTRIBUTE_DIRECTORY))
		{
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
