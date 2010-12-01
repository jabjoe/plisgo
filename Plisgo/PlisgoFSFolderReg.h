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

	bool						IsRootValidForPath(IPtrPlisgoFSRoot root, const std::wstring& rsPath) const;


	typedef boost::unordered_map<std::wstring, IPtrPlisgoFSRoot>	RootCacheMap;			

	RootCacheMap					m_RootCache;

	mutable boost::shared_mutex		m_Mutex;
};