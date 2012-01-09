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
#include "Userenv.h"
#include "../Common/RegUtils.h"

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


static bool __stdcall CollectSIDsCB(LPCWSTR sKey, WStringList* pList)
{
	pList->push_back(sKey);

	return true;
}


void CleanPlisgoTempFolders()
{
	WStringList sids;

	EnumRegKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList", (EnumRegKeyCB)CollectSIDsCB, &sids);

	std::wstring sProfileKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\";
	size_t sProfileKeyBaseLen = sProfileKey.length();

	std::wstring sEnviromentKey;

	std::wstring sProfileFolder;
	std::wstring sTempKey;

	WCHAR sWindows[MAX_PATH] = {0};

	GetWindowsDirectory(sWindows, MAX_PATH);

	WCHAR sDrive[3];

	wcsncpy_s(sDrive, 3, sWindows, 2);


	for(WStringList::const_iterator it = sids.begin(); it != sids.end(); ++it)
	{
		sProfileKey.resize(sProfileKeyBaseLen);
		sProfileKey += *it;

		sProfileFolder.clear();

		if (LoadStringFromReg(HKEY_LOCAL_MACHINE, sProfileKey.c_str(), L"ProfileImagePath", sProfileFolder))
		{
			sEnviromentKey = *it;
			sEnviromentKey += L"\\Environment";

			if (LoadStringFromReg(HKEY_USERS, sEnviromentKey.c_str(), L"TEMP", sTempKey))
			{
				boost::algorithm::ireplace_all(sTempKey, L"%USERPROFILE%", sProfileFolder);
				boost::algorithm::ireplace_all(sTempKey, L"%SYSTEMROOT%", sWindows);
				boost::algorithm::ireplace_all(sTempKey, L"%SYSTEMDRIVE%", sDrive);

				sTempKey += L"\\plisgo*";
				sTempKey += L'\0'; //Add double termination for SHFileOperation

				SHFILEOPSTRUCTW fileOp = {0};

				fileOp.wFunc = FO_DELETE;
				fileOp.pFrom = sTempKey.c_str();
				fileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI ;

				SHFileOperationW(&fileOp);
			}
		}
	}
}


// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
   HRESULT hr = _Module.RegisterServer(FALSE);

   if (hr == S_OK)
   {
	   WCHAR sDLLPath[MAX_PATH];

	   HMODULE h = GetModuleHandleW(L"Plisgo.dll");

	   GetModuleFileNameW(h, sDLLPath, MAX_PATH);

	   WCHAR sSystemPath[MAX_PATH];

	   GetSystemDirectory(sSystemPath, MAX_PATH);

	   std::wstring sEntry = sSystemPath;
	   
	   sEntry += L"\\rundll32.exe \"";
	   sEntry += sDLLPath;
	   sEntry += L"\",CleanPlisgoTempFolders";

	   SaveStringToReg(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", L"PlisgoClean", sEntry);
   }

   return hr;
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
	HRESULT hr = _Module.UnregisterServer(FALSE);

	if (hr == S_OK)
	{
		HKEY hRegHandle = NULL;
		
		RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hRegHandle);
		
		if (hRegHandle != NULL && hRegHandle != INVALID_HANDLE_VALUE)
		{
			RegDeleteValue(hRegHandle, L"PlisgoClean");
			RegCloseKey(hRegHandle);
		}
	}

	return hr;
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
