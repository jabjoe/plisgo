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

extern HICON		GetSpecificIcon( const std::wstring& rsFile, const int nIndex, const UINT nHeight);

extern HBITMAP		CreateAlphaBitmap(HDC hDC, LONG nWidth, LONG nHeight, DWORD** ppBits = NULL);

extern HBITMAP		ExtractAsBitmap(HINSTANCE hHandle, LPCWSTR sName, LPCWSTR sType, UINT* pnHeight = NULL);

extern HICON		CreateIconFromBitMap(HBITMAP hBitmap, const UINT nAimHeight);

extern HICON		EnsureIconSizeResolution(HICON hIcon, LONG nHeight);

extern HICON		BurnTogether(HICON hFirst, POINT &rFirst, HICON hSecond, POINT &rSecond, UINT nHeight, bool bOutline = false);

extern HICON		LoadAsIcon(const std::wstring& rsFile, UINT nHeight);

extern bool			ExtractIconInfoForExt( std::wstring& rsIconFilePath, int& rnIconIndex, LPCWSTR sExt );

extern int			AddIconToResourcedFile(const std::wstring& rsFile, const HICON hIcon);

extern bool			WriteToIconFile(const std::wstring& rsFile, HICON hIcon, bool bAppend = true);

extern HICON		ExtractIconFromImageListFile(const std::wstring& rsFile, const UINT nIndex, const UINT nAimHeight);

extern HIMAGELIST	LoadImageList(const std::wstring& rsFile, const UINT nHeight);
