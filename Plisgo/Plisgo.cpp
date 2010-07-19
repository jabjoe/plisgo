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
#include "resource.h"
#include "dllmain.h"


// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}


// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}


// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
   return _Module.RegisterServer(FALSE);
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
	return _Module.UnregisterServer(FALSE);
}


HRESULT		GetShellIShellFolder2Implimentation(IShellFolder2** ppResult)
{
	IUnknown* pUnknown;

	HRESULT nResult = CoGetClassObject(CLSID_ShellFSFolder, CLSCTX_INPROC_SERVER , 0, IID_IUnknown, (void**)&pUnknown);

	if ( !FAILED(nResult) )
	{
		CComQIPtr<IClassFactory> pFactory(pUnknown);

		pUnknown->Release();

		if ( pFactory.p != NULL )
			nResult = pFactory->CreateInstance(NULL, IID_IShellFolder2, (void**)ppResult);
	}

	return nResult;
}


void PrintInterfaceName(REFIID riid)
{
	OLECHAR szName[128];
	*szName = 0;
	HKEY hkey;
	
	// open the Interface (IID) key
	LONG r = RegOpenKeyEx( HKEY_CLASSES_ROOT, __TEXT("Interface"),
		0, KEY_QUERY_VALUE, &hkey);
	
	if (r == ERROR_SUCCESS)
	{
		OLECHAR szGuid[64];
		// convert IID to a string (unicode)
		StringFromGUID2(riid, szGuid, sizeof(szGuid));
		
		// read value at corresponding key
		long cb;
		r = RegQueryValueW(hkey, szGuid, szName, &cb);
		if (r == ERROR_SUCCESS)
		{
			OutputDebugStringW(szName);
		}
		else
		{
			OutputDebugStringW(szGuid);
		}
		OutputDebugStringA("\n");
		RegCloseKey(hkey);
	}
}
