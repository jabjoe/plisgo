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

#ifndef STRICT
#define STRICT
#endif

#include "targetver.h"

static const int PLISGO_APIVERSION = 2;

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#include "resource.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <shlobj.h>
#include <gdiplus.h>
#include <CommonControls.h>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <algorithm>
#include <assert.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/format.hpp>
#include <fstream>
#include <unordered_map>
#include "../Common/Utils.h"
#include "../Common/PathUtils.h"
#include "../Common/IconUtils.h"

using namespace ATL;


#include "resource.h"
#include "Plisgo_i.h"
#include "dllmain.h"


#ifdef UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif



static UINT WM_PLISGOVIEWSHELLMESSAGE = RegisterWindowMessage(_T("PlisgoViewShellMessage"));

const ULONG64 NTSECOND = 10000000;
const ULONG64 NTMINUTE = NTSECOND*60;


class PlisgoFSRoot;

typedef boost::shared_ptr<PlisgoFSRoot>	IPtrPlisgoFSRoot;

class PlisgoFSLocal;

typedef boost::shared_ptr<PlisgoFSLocal>	IPtrPlisgoFSLocal;

class PlisgoFSMenu;

class IPtrPlisgoFSMenu;

class FSIconRegistry;

typedef boost::shared_ptr<FSIconRegistry>	IPtrFSIconRegistry;

class RefIconList;

typedef boost::shared_ptr<RefIconList>	IPtrRefIconList;

class IconRegistry;

typedef std::vector<std::wstring>	WStringList;


inline void		FillLowerCopy(WCHAR* sDst, size_t nDstSize, const WCHAR* sSrc)
{
	assert(sDst != NULL && sSrc != NULL);

	--nDstSize; //Ensure space for \0.

	while(*sSrc != L'\0' && nDstSize > 0)
	{
		*sDst++ = tolower(*sSrc++);
		--nDstSize;
	}

	*sDst = L'\0';
}


inline bool			ReadIndicesPair(CHAR* sBuffer, UINT& rnList, UINT& rnEntry)
{
	assert(sBuffer != NULL);

	CHAR* sDevide = strchr(sBuffer,':');

	if (sDevide != NULL)
	{
		*sDevide++ = 0;

		rnList	= (UINT)atol(sBuffer);
		rnEntry	= (UINT)atol(sDevide);
		
		return true;
	}

	return false;
}


inline int			PrePathCharacter(int c)
{
	if (c == L'/')
		return L'\\';

	return tolower(c);
};


inline void		AddToPreHash64(ULONG64& rnHash, int n)
{
	rnHash += n;
	rnHash += (rnHash << 10);
	rnHash ^= (rnHash >> 6);
}

inline ULONG64	HashFromPreHash(ULONG64 nHash)
{
	nHash += (nHash << 3);
	nHash ^= (nHash >> 11);
	nHash += (nHash << 15);

	return nHash;
}


inline ULONG64		SimpleHash64(const wchar_t* sKey)
{
	ULONG64 nResult = 0;

	if (sKey)
		for(; *sKey != L'\0'; ++sKey)
			AddToPreHash64(nResult, tolower(*sKey));

	return HashFromPreHash(nResult);
}

extern HRESULT		GetShellIShellFolder2Implimentation(IShellFolder2** ppResult);

extern void			PrintInterfaceName(REFIID riid);


#define HRESULTTOLRESULT(_hr) (_hr&0x7FFFFFFF)
#define LRESULTTOHRESULT(_lr) ((HRESULT)(0x80000000 | ((DWORD)_lr >> 1)))


inline void			ToWide(std::wstring& rDst, const char* sSrc)
{
	if (sSrc == NULL)
	{
		rDst.clear();
		return;
	}

	int nLen = (int)strlen(sSrc);

	int nSize = MultiByteToWideChar(CP_UTF8, 0, sSrc, nLen, NULL, 0);

	rDst.assign(nSize+1, L' ');

	nSize = MultiByteToWideChar(CP_UTF8, 0, sSrc, nLen, const_cast<WCHAR*>(rDst.c_str()), (int)rDst.length());

	rDst.resize(nSize);
}


#define TOPOWEROFTWO(_x) ((_x%2)?_x+1:_x)