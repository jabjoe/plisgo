/*
	Copyright 2009 Eurocom Entertainment Software Limited

    This file is part of PlisgoFSLib.

    Eurocom Entertainment Software Limited, hereby disclaims all
    copyright interest in “PlisgoFSLib” written by Joe Burmeister.

    PlisgoFSLib is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

    PlisgoFSLib is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
	License along with PlisgoFSLib.  If not, see
	<http://www.gnu.org/licenses/>.
*/

#pragma once

#define WIN32_EXTRA_LEAN
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/unordered_map.hpp>


static const int PLISGO_APIVERSION = 2;

#define PLISGOFSLIB_VERSION 1.1.0.0


typedef	WCHAR		FileNameBuffer[MAX_PATH];

extern bool			WildCardMatch(LPCWSTR* sMask, LPCWSTR* sTarget);

extern void			FromWide(std::string& rsDst, const std::wstring& sSrc);
extern void			ToWide(std::wstring& rDst, const std::string& sSrc);

class StringEvent
{
public:
	virtual ~StringEvent() {}
	virtual bool Do(LPCWSTR) = 0;
};

typedef boost::shared_ptr<StringEvent>	IPtrStringEvent;



inline size_t				CopyToLower(LPWSTR sDst, size_t nDstSize, LPCWSTR sSrc)
{
	assert(sSrc != NULL && sDst != NULL && nDstSize > 0);

	size_t nLen = 0;

	--nDstSize;

	for(; sSrc[nLen] != 0 && nLen < nDstSize; ++nLen)
		sDst[nLen] = (WCHAR)tolower(sSrc[nLen]);

	sDst[nLen] = 0;

	return nLen;
}


inline size_t				CopyToLower(FileNameBuffer sDst, LPCWSTR sSrc)
{
	return CopyToLower(sDst, MAX_PATH, sSrc);
}


inline bool					ParseLower(LPCWSTR& rsToParse, LPCWSTR sAgainst)
{
	assert(rsToParse != NULL && sAgainst != NULL);

	for(;*sAgainst != 0 && *rsToParse != 0 &&
		*sAgainst == tolower(*rsToParse);
		++rsToParse,++sAgainst);

	return (*sAgainst == 0);
}


inline bool					ParseNoCase(LPCWSTR& rsToParse, LPCWSTR sAgainst)
{
	assert(rsToParse != NULL && sAgainst != NULL);

	for(;*sAgainst != 0 && *rsToParse != 0 &&
		tolower(*sAgainst) == tolower(*rsToParse);
		++rsToParse,++sAgainst);

	return (*sAgainst == 0);
}


inline bool					MatchLower(LPCWSTR sMatch, LPCWSTR sAgainst)
{
	LPCWSTR sTemp = sMatch;

	return (ParseLower(sTemp, sAgainst) && sTemp[0] == L'\0');		
}


inline LPCWSTR				GetNameFromPath(LPCWSTR sPath)
{
	assert(sPath != NULL);

	LPCWSTR sResult = sPath;

	for(++sPath;*sPath != L'\0'; ++sPath)
	{
		if (sPath[-1] == L'\\')
			sResult = sPath;
	}

	return sResult;
}


inline LPCWSTR				GetNameFromPath(const std::wstring& rsPath)
{
	const size_t nSlash = rsPath.rfind(L'\\');

	return &rsPath.c_str()[nSlash+1];
}


inline LPCWSTR				GetParentFromPath(LPCWSTR sPath, LPCWSTR sName)
{
	LPCWSTR sParent = sName-2; //Skip past slash

	while(sParent > sPath && sParent[0] != L'\\')
		--sParent;

	if (sParent[0] == L'\\')
		++sParent;

	return sParent;
}


inline void					MakePathHashSafe(std::wstring& rsPath)
{
	std::transform(rsPath.begin(),rsPath.end(),rsPath.begin(),tolower);

	boost::trim_right_if(rsPath, boost::is_any_of(L"\\"));
	boost::trim_left_if(rsPath, boost::is_any_of(L"\\"));
}

