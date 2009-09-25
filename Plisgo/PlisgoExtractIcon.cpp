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




void CPlisgoExtractIcon::Init(const std::wstring& sPath, IPtrPlisgoFSRoot localFSFolder)
{
	assert(sPath.length() > 0 && localFSFolder.get() != NULL);

	m_PlisgoFSFolder = localFSFolder;

	m_sPath = sPath;
}


STDMETHODIMP CPlisgoExtractIcon::GetIconLocation( UINT uFlags, LPWSTR szIconFile, UINT cchMax, int* piIndex, UINT* pwFlags )
{
	if (piIndex == NULL)
		return ERROR_INVALID_PARAMETER;

	bool bOpen = (uFlags&GIL_OPENICON)?true:false;

	IPtrRefIconList iconList = m_PlisgoFSFolder->GetFSIconRegistry()->GetMainIconRegistry()->GetRefIconList(16);

	if (iconList.get() == NULL)
		return ERROR_NOT_SUPPORTED;

	IconLocation Location;

	*pwFlags = 0; //Let Windows do (and cache) the icon extraction.

	if (m_PlisgoFSFolder->GetPathIconLocation(Location, m_sPath, iconList, bOpen))
	{
		wcscpy_s(szIconFile, cchMax, Location.sPath.c_str());

		*piIndex = Location.nIndex;

		return E_PENDING;
	}

	return E_FAIL;
}
