/*
	Copyright 2009 Eurocom Entertainment Software Limited

    This file is part of Plisgo's Utils.

    Eurocom Entertainment Software Limited, hereby disclaims all
    copyright interest in “Plisgo's Utils” written by Joe Burmeister.

    PlisgoFSLib is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

    Plisgo's Utils is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
	License along with Plisgo's Utils.  If not, see
	<http://www.gnu.org/licenses/>.
*/

#pragma once


extern bool			ExtIsCodeImage(LPCWSTR sExt);
extern bool			ExtIsShortcut(LPCWSTR sExt);
extern bool			ExtIsShortcutUrl(LPCWSTR sExt);
extern bool			ExtIsIconFile(LPCWSTR sExt);


inline void			EnsureWin32Path(std::wstring& rsPath)
{
	size_t n = rsPath.length();

	while(n--)
		if (rsPath[n] == L'/')
			rsPath[n] = L'\\';
}

extern void			EnsureFullPath(std::wstring& rsFile);

extern bool			ReadTextFromFile(std::wstring& rsResult, LPCWSTR sFile);
extern bool			ReadIntFromFile(int& rnResult, LPCWSTR sFile);
extern bool			ReadDoubleFromFile(double& rnResult, LPCWSTR sFile);

extern HRESULT		GetWStringPathFromIDL(std::wstring& rResult, LPCITEMIDLIST pIDL);