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

#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <string>
#include <CommonControls.h>
#include <gdiplus.h>
#include <shlobj.h>
#include <boost/algorithm/string/trim.hpp>
#include <vector>

extern HINSTANCE g_hInstance;

#define GETOFFSET(_struct, _member)	((size_t)&((_struct*)NULL)->_member)