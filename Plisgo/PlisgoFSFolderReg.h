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

#pragma once

#include "PlisgoFSFolder.h"


class PlisgoFSFolderReg
{
public:

	static PlisgoFSFolderReg*	GetSingleton();

	IPtrPlisgoFSRoot			GetPlisgoFSRoot(LPCWSTR sPath) const;

	void						RunRootCacheClean();

private:
	PlisgoFSFolderReg();

	IPtrPlisgoFSRoot			CreateRoot(const std::wstring& rsRoot);
	IPtrPlisgoFSRoot			GetPlisgoFSRoot_Locked(const std::wstring& rsPath, const std::wstring& rsSection);

	bool						IsRootValidForPath(const std::wstring& rsRootPath, const std::wstring& rsPath) const;

	struct RootHolder
	{
		IPtrPlisgoFSRoot	root;
		RootHolder			*pNext;
		RootHolder			*pPrev;
		ULONG64				nLastCheckTime; //Last time the root was checked
	};

	struct PathRootCacheEntry
	{
		boost::weak_ptr<PlisgoFSRoot>	root;
		ULONG64							nLastCheckTime; //Last time the path was checked again the root
	};

	typedef std::pair<std::wstring, PathRootCacheEntry>				RootCacheMapEntry;
	typedef boost::fast_pool_allocator< RootCacheMapEntry >			RootCacheMapEntryPool;
	typedef boost::unordered_map<std::wstring,
								PathRootCacheEntry,
								boost::hash<std::wstring>,
								std::equal_to<std::wstring>,
								RootCacheMapEntryPool >				RootCacheMap;

	typedef boost::object_pool< RootHolder >				RootHolderPool;

	RootCacheMap					m_RootCache;

	RootHolder*						m_pOldestRootHolder;
	RootHolder*						m_pNewestRootHolder;
	RootHolderPool					m_RootHolderPool;

	mutable boost::shared_mutex		m_Mutex;
};