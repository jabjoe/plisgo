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
#include "Plisgo_i.h"
#include "dllmain.h"

// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    return _AtlModule.DllCanUnloadNow();
}


// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}


// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    HRESULT hr = _AtlModule.DllRegisterServer();
	return hr;
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
	HRESULT hr = _AtlModule.DllUnregisterServer();
	return hr;
}

// DllInstall - Adds/Removes entries to the system registry per user
//              per machine.	
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    HRESULT hr = E_FAIL;
    static const wchar_t szUserSwitch[] = _T("user");

    if (pszCmdLine != NULL)
    {
    	if (_wcsnicmp(pszCmdLine, szUserSwitch, _countof(szUserSwitch)) == 0)
    	{
    		AtlSetPerUserRegistration(true);
    	}
    }

    if (bInstall)
    {	
    	hr = DllRegisterServer();
    	if (FAILED(hr))
    	{	
    		DllUnregisterServer();
    	}
    }
    else
    {
    	hr = DllUnregisterServer();
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



bool	ReadTextFromFile(std::wstring& rsResult, LPCWSTR sFile)
{
	HANDLE hFile = CreateFileW(sFile, FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
		return false;
	
	bool bResult = false;

	LARGE_INTEGER nSize;

	nSize.LowPart = GetFileSize(hFile, (LPDWORD)&nSize.HighPart);

	if (nSize.HighPart == 0 && nSize.LowPart < 1024*1024) //Check it's not bigger than a meg
	{
		rsResult.reserve((unsigned int)nSize.QuadPart);

		CHAR buffer[MAX_PATH];

		DWORD nRead = 0;


		while(ReadFile(hFile, buffer, MAX_PATH-1, &nRead, NULL) && nRead > 0)
		{
			buffer[nRead] = 0;

			DWORD nWRead = MultiByteToWideChar(CP_UTF8, 0, buffer, (int)nRead, NULL, 0);

			WCHAR* wBuffer = (WCHAR*)_malloca(sizeof(WCHAR)*(nWRead+1));

			nWRead = MultiByteToWideChar(CP_UTF8, 0, buffer, (int)nRead, wBuffer, nWRead);

			wBuffer[nWRead] = 0;

			rsResult += wBuffer;
			bResult = true;

			_freea(wBuffer);
		}
	}

	CloseHandle(hFile);

	return bResult;
}


bool	ReadIntFromFile(int& rnResult, LPCWSTR sFile)
{
	HANDLE hFile = CreateFileW(sFile, FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
		return false;

	CHAR buffer[MAX_PATH];

	DWORD nRead = 0;

	bool bResult = (ReadFile(hFile, buffer, MAX_PATH-1, &nRead, NULL) == TRUE);

	if (bResult)
	{
		buffer[nRead] = 0;

		rnResult = atoi(buffer);
	}

	CloseHandle(hFile);

	return true;
}


bool	ReadDoubleFromFile(double& rnResult, LPCWSTR sFile)
{
	HANDLE hFile = CreateFileW(sFile, FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
		return false;

	CHAR buffer[MAX_PATH];

	DWORD nRead = 0;

	bool bResult = (ReadFile(hFile, buffer, MAX_PATH-1, &nRead, NULL) == TRUE);

	if (bResult)
	{
		buffer[nRead] = 0;

		rnResult = atof(buffer);
	}

	CloseHandle(hFile);

	return true;
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
