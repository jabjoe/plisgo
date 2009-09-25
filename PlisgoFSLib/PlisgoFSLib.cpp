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

#include "PlisgoFSLib.h"





static LPCWSTR FindNextWildCard(LPCWSTR sMask)
{
	if (*sMask)
	{
		while(*sMask != 0)
		{
			if (*sMask == L'*')
				return sMask;

			++sMask;
		}

		return sMask;
	}

	return NULL;
}




bool WildCardMatch(LPCWSTR sMask, LPCWSTR sTarget)
{
	LPCWSTR sPos = wcschr(sMask, L'*');

	if (sPos == NULL)
	{
		for(WCHAR m = *sMask++, t = *sTarget++; m != 0; m = *sMask++, t = *sTarget++)
			if (tolower(m) != tolower(t))
				return false;

		return true;
	}

	while(*sPos == L'*')
		++sPos;

	if (*sPos == 0)
		return true;

	LPCWSTR sNextPos = FindNextWildCard(sPos);

	size_t nTargetPos = 0;

	WCHAR buffer[512];


	while (sNextPos != NULL)
	{
		size_t nSectionLen = CopyToLower(buffer, 512, sPos);

		size_t nMatching = 0;

		for(; sTarget[nTargetPos] != 0 && nMatching != nSectionLen; ++nTargetPos )
		{
			if (tolower(sTarget[nTargetPos]) == buffer[nMatching])
				++nMatching;
			else
				nMatching = 0;
		}
		
		if (nMatching != nSectionLen)
			return false;
		
		sPos		= sNextPos;

		while(*sPos == L'*')
			++sPos;

		sNextPos	= FindNextWildCard(sPos);
	}

	//Both should be at a end if there is a match
	return (sTarget[nTargetPos] == *sPos);
}


void			FromWide(std::string& rsDst, const std::wstring& sSrc)
{
	int nSize = WideCharToMultiByte(CP_UTF8, 0, sSrc.c_str(), (int)sSrc.length(), NULL, 0, NULL, NULL);
	
	rsDst.assign(nSize+1, ' ');

	nSize = WideCharToMultiByte(CP_UTF8, 0, sSrc.c_str(), (int)sSrc.length(), const_cast<char*>(rsDst.c_str()), (int)rsDst.length(), NULL, NULL);

	rsDst.resize(nSize);
}


void			ToWide(std::wstring& rDst, const std::string& sSrc)
{
	int nSize = MultiByteToWideChar(CP_UTF8, 0, sSrc.c_str(), (int)sSrc.length(), NULL, 0);

	rDst.assign(nSize+1, L' ');

	nSize = MultiByteToWideChar(CP_UTF8, 0, sSrc.c_str(), (int)sSrc.length(), const_cast<WCHAR*>(rDst.c_str()), (int)rDst.length());

	rDst.resize(nSize);
}
