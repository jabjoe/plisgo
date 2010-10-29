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
#include "PlisgoExtractIcon.h"
#include "IconRegistry.h"

const CLSID CLSID_PlisgoExtractIcon =		{0x11E54957,0x7EED,0x4F3C,{0xA4,0xC1,0x46,0x78,0x87,0x57,0xEC,0x62}};



void CPlisgoExtractIcon::Init(const std::wstring& sPath, IPtrPlisgoFSRoot localFSFolder)
{
	assert(sPath.length() > 0 && localFSFolder.get() != NULL);

	m_PlisgoFSFolder = localFSFolder;

	m_sPath = sPath;
}


STDMETHODIMP CPlisgoExtractIcon::GetIconLocation( UINT uFlags, LPWSTR szIconFile, UINT cchMax, int* piIndex, UINT* pwFlags )
{
	if (piIndex == NULL || szIconFile == NULL)
		return ERROR_INVALID_PARAMETER;

	bool bOpen = (uFlags&GIL_OPENICON)?true:false;

	IconLocation Location;

	if (m_PlisgoFSFolder->GetPathIconLocation(Location, m_sPath, bOpen))
	{
		if (Location.sPath.length() < cchMax)
		{
			wcscpy_s(szIconFile, cchMax, Location.sPath.c_str());

			*piIndex = Location.nIndex;

			*pwFlags = 0; //Let Windows do (and cache) the icon extraction.

			return S_OK;
		}
	}

	return E_FAIL;
}


STDMETHODIMP CPlisgoExtractIcon::Extract(	LPCTSTR pszFile, UINT nIconIndex, HICON* pHiconLarge,
											HICON* pHiconSmall, UINT /*nIconSize*/ )
{
	return S_FALSE; //Get Windows to do it!

	/*if (!m_bExtract)
		return S_FALSE;

	IconLocation Location;

	if (!m_PlisgoFSFolder->GetPathIconLocation(Location, m_sPath, m_bOpen))
		return E_FAIL;

	if (pHiconLarge == NULL && pHiconSmall == NULL)
		return S_OK; //Nothing to do

	HICON hIcon = ExtractIcon(g_hInstance, Location.sPath.c_str(), Location.nIndex);

	if (pHiconLarge != NULL && pHiconSmall != NULL)
	{
		*pHiconLarge = hIcon;
		*pHiconSmall = ExtractIcon(g_hInstance, Location.sPath.c_str(), Location.nIndex);
	}
	else if (pHiconSmall != NULL)
	{
		*pHiconSmall = hIcon;
	}
	else if (pHiconLarge != NULL)
	{
		*pHiconLarge = hIcon;
	}

	return S_OK;*/


}