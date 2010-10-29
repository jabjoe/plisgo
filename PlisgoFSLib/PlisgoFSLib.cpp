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
#include "Common/RegUtils.h"




void			FromWide(std::string& rsDst, const std::wstring& sSrc)
{
	int nSize = WideCharToMultiByte(CP_UTF8, 0, sSrc.c_str(), (int)sSrc.length(), NULL, 0, NULL, NULL);
	
	rsDst.assign(nSize+1, ' ');

	nSize = WideCharToMultiByte(CP_UTF8, 0, sSrc.c_str(), (int)sSrc.length(), const_cast<char*>(rsDst.c_str()), (int)rsDst.length(), NULL, NULL);

	rsDst.resize(nSize);
}


void			FromWide(std::string& rsDst, LPCWSTR sSrc)
{
	if (sSrc == NULL)
	{
		rsDst.clear();
		return;
	}

	int nLen = (int)wcslen(sSrc);

	int nSize = WideCharToMultiByte(CP_UTF8, 0, sSrc, nLen, NULL, 0, NULL, NULL);
	
	rsDst.assign(nSize+1, ' ');

	nSize = WideCharToMultiByte(CP_UTF8, 0, sSrc, nLen, const_cast<char*>(rsDst.c_str()), (int)rsDst.length(), NULL, NULL);

	rsDst.resize(nSize);
}


void			ToWide(std::wstring& rDst, const std::string& sSrc)
{
	int nSize = MultiByteToWideChar(CP_UTF8, 0, sSrc.c_str(), (int)sSrc.length(), NULL, 0);

	rDst.assign(nSize+1, L' ');

	nSize = MultiByteToWideChar(CP_UTF8, 0, sSrc.c_str(), (int)sSrc.length(), const_cast<WCHAR*>(rDst.c_str()), (int)rDst.length());

	rDst.resize(nSize);
}


void			ToWide(std::wstring& rDst, const char* sSrc)
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


class PlisgoAPIVersionRetriver
{
public:
	PlisgoAPIVersionRetriver()
	{
		m_nVersion = PLISGO_MIN_APIVERSION;

		//Do we need to step down?

		std::wstring sPlisgo;

		//Note, on a 64bit machine, this will get the 32bit DLL, that should also be registered

		if (LoadStringFromReg(HKEY_CLASSES_ROOT, L"CLSID\\{ADA19F85-EEB6-46F2-B8B2-2BD977934A79}\\InprocServer32", NULL, sPlisgo))
		{
			HMODULE hModule = LoadLibraryW(sPlisgo.c_str());

			if (hModule != NULL)
			{
				typedef DWORD (WINAPI *LPFN_MAXAPIVERSION)();

				LPFN_MAXAPIVERSION cb = (LPFN_MAXAPIVERSION)GetProcAddress(hModule, "GetMaxPlisgoAPIVersion");

				if (cb != NULL)
				{
					DWORD nMaxVersion = cb();

					if (nMaxVersion > PLISGO_MAX_APIVERSION)
						m_nVersion = PLISGO_MAX_APIVERSION; //We this is as high as we go
					else
						m_nVersion = nMaxVersion; //We can match you
				}

				FreeLibrary(hModule);
			}
		}
	};

	DWORD	GetVersion() const { return m_nVersion; }

private:
	DWORD	m_nVersion;
};



DWORD		GetPlisgoAPIVersion()
{
	static PlisgoAPIVersionRetriver retriver;

	return retriver.GetVersion();
}