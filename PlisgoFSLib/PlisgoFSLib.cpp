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
