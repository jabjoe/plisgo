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


bool	LoadDWORDFromReg(HKEY hRootKey, LPCWSTR sKey, LPCWSTR sName, DWORD& rnValue, bool bDoWOW64 = false);
bool	SaveDWORDToReg(HKEY hRootKey, LPCWSTR sKey, LPCWSTR sName, DWORD nValue, bool bDoWOW64 = false);

bool	LoadStringFromReg(HKEY hRootKey, LPCWSTR sKey, LPCWSTR sName, std::wstring& rsValue, bool bDoWOW64 = false);
bool	SaveStringToReg(HKEY hRootKey, LPCWSTR sKey, LPCWSTR sName, const std::wstring& rsValue, bool bDoWOW64 = false);

typedef bool (__stdcall *EnumRegKeyCB)(LPCWSTR sKey, void* pData);

bool	EnumRegKey(HKEY hRootKey, LPCWSTR sKey, EnumRegKeyCB cd, void* pData, bool bDoWOW64 = false);
