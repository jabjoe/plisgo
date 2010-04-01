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
#include "PlisgoFSFolderReg.h"

CPlisgoModule _AtlModule;


HINSTANCE g_hInstance = NULL;



// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	static volatile LONG nThreadNum = 0;

	switch(dwReason)
	{
	case DLL_PROCESS_ATTACH: g_hInstance = hInstance; break;
	case DLL_THREAD_ATTACH: InterlockedIncrement(&nThreadNum); break;
	case DLL_THREAD_DETACH: InterlockedDecrement(&nThreadNum); break;
	case DLL_PROCESS_DETACH: g_hInstance = NULL;  break;
	}

	return _AtlModule.DllMain(dwReason, lpReserved); 
}
