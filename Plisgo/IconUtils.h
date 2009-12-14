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


extern HICON		GetSpecificIcon( const std::wstring& rsFile, const int nIndex, const UINT nHeight);

extern HBITMAP		CreateAlphaBitmap(HDC hDC, LONG nWidth, LONG nHeight, DWORD** ppBits = NULL);

extern HICON		EnsureIconSizeResolution(HICON hIcon, LONG nHeight);

extern HICON		BurnTogether(HICON hFirst, HICON hSecond, UINT nHeight);

extern HICON		LoadAsIcon(const std::wstring& rsFile, UINT nHeight);

extern bool			ExtractIconInfoForExt( std::wstring& rsIconFilePath, int& rnIconIndex, LPCWSTR sExt );

extern int			AddIconToResourcedFile(const std::wstring& rsFile, const HICON hIcon);

extern bool			WriteToIconFile(const std::wstring& rsFile, HICON hIcon, bool bAppend = true);

extern bool			ExtIsCodeImage(LPCWSTR sExt);
extern bool			ExtIsShortcut(LPCWSTR sExt);
extern bool			ExtIsShortcutUrl(LPCWSTR sExt);
extern bool			ExtIsIconFile(LPCWSTR sExt);

extern HICON		ExtractIconFromImageListFile(const std::wstring& rsFile, const UINT nIndex, const UINT nAimHeight);
extern HIMAGELIST	LoadImageList(const std::wstring& rsFile, const UINT nHeight);
