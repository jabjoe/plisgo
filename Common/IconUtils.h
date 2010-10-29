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

extern HBITMAP		ExtractAsBitmap(HINSTANCE hHandle, LPCWSTR sName, LPCWSTR sType, UINT* pnHeight = NULL);

extern HBITMAP		ExtractBitmap( const std::wstring& rsFile, LONG nWidth, LONG nHeight, WORD nDepth);


extern HICON		CreateIconFromBitMap(HBITMAP hBitmap, const UINT nAimHeight);

extern HICON		EnsureIconSizeResolution(HICON hIcon, LONG nHeight);

extern HICON		BurnTogether(HICON hFirst, POINT &rFirst, HICON hSecond, POINT &rSecond, UINT nHeight, bool bOutline = false);

extern HICON		LoadAsIcon(const std::wstring& rsFile, UINT nHeight);

extern bool			ExtractIconInfoForExt( std::wstring& rsIconFilePath, int& rnIconIndex, LPCWSTR sExt );

extern bool			ExtractOwnIconInfoOfFile( std::wstring& rsIconFilePath, int& rnIconIndex, const std::wstring& rsFile);

extern int			AddIconToResourcedFile(const std::wstring& rsFile, const HICON hIcon);

extern bool			WriteToIconFile(const std::wstring& rsFile, HICON hIcon, bool bAppend = true);
extern bool			WriteToIconFile(const std::wstring& rsFile, const HICON* phIcons, int nIconNum, bool bAppend = true);

extern HICON		ExtractIconFromImageListFile(const std::wstring& rsFile, const UINT nIndex, const UINT nAimHeight);

extern HIMAGELIST	LoadImageList(const std::wstring& rsFile, const UINT nHeight);

extern HIMAGELIST	LoadImageList(const std::wstring& rsFile);

extern HICON		ExtractIconFromFile(const std::wstring& rsFile, const UINT nIndex);

extern HICON		GetSpecificIcon( LPCWSTR sFile, const int nIndex);

extern HBITMAP		GetIconAsRGBABitMap(HICON hIcon, DWORD nWidth, DWORD nHeight);

extern bool			BurnIconsTogether(LPCWSTR sDstFile, LPCWSTR sBaseFile, int nBaseIconIndex, LPCWSTR sOverlayFile, int nOverlayIconIndex);


