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

			WCHAR wBuffer[MAX_PATH];

			nRead = MultiByteToWideChar(CP_UTF8, 0, buffer, (int)nRead, wBuffer, MAX_PATH);

			wBuffer[nRead] = 0;

			rsResult += wBuffer;
			bResult = true;

			SetFilePointer(hFile, nRead, 0, FILE_CURRENT);
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


static void AppendANSI(std::wstring& rResult, const char* sStr)
{
	int nAnsiLen = strlen(sStr);

	int nWideLen = MultiByteToWideChar(CP_UTF8, 0, sStr, nAnsiLen, NULL, 0);

	size_t nPos = rResult.length();

	rResult.resize(nPos+nWideLen);

	MultiByteToWideChar(CP_UTF8, 0, sStr, nAnsiLen, (WCHAR*)&rResult.c_str()[nPos], nWideLen);
}


static void SlashEndingEnfore(std::wstring& rResult)
{
	if (rResult.length() > 0)
		if (rResult[rResult.length()-1] != L'\\')
			rResult += L'\\';
}



HRESULT GetWStringPathFromIDL(std::wstring& rResult, LPCITEMIDLIST pIDL)
{
	//THIS IS DANAGERESS MEMORY EVIL, but it's fastest way by a long shot. Doesn't work on Win7 yet
/*
	LPCITEMIDLIST pCurrentIDL = pIDL;

	while(pCurrentIDL->mkid.cb != 0)
	{
		switch(pCurrentIDL->mkid.abID[0])
		{
		case 0x23:
		case 0x25:
		case 0x29:
		case 0x2f:
			//Ansi drive with path
			{
				const size_t nPos = rResult.size();

				AppendANSI(rResult, (const char*)&pCurrentIDL->mkid.abID[1]);
				
				assert(rResult.size() > nPos+2);

				rResult[nPos+2] = L'\\';
			}
			break;
		case 0x30:
		case 0x31:
		case 0x32:
		case 0xb1:
			// file structure
			// Read http://source.winehq.org/source//dlls/shell32/pidl.h
			{
				const BYTE* pPos = &pCurrentIDL->mkid.abID[12];

				const char* sAnsiName = (const char*)pPos;
			
				const BYTE* pEnd = ((const BYTE*)pCurrentIDL) + pCurrentIDL->mkid.cb;

				const WORD nNTOffset = *(WORD*)(pEnd-2);
				
				pPos += strlen(sAnsiName);

				if (nNTOffset != (WORD)(pPos-(BYTE *)pCurrentIDL) )
				{
					//Second string
					pPos += strlen((const char*)pPos)+1;
				}

				if (pPos-(BYTE *)pCurrentIDL == nNTOffset-1)
					++pPos;

				SlashEndingEnfore(rResult);

				if (pEnd-pPos>0 && nNTOffset == pPos-(BYTE *)pCurrentIDL)
				{
					const wchar_t* sWidePath = (const wchar_t*)(((const char*)pPos) + 20);

					rResult += sWidePath;
				}
				else
				{
					AppendANSI(rResult, sAnsiName);
				}
			}
			break;
		case 0x34:
			// Wide file structure
			{
				const wchar_t* sWidePath = (const wchar_t*)&pCurrentIDL->mkid.abID[20];

				SlashEndingEnfore(rResult);

				rResult += sWidePath;
			}
			break;
		}

		pCurrentIDL = ILGetNext(pCurrentIDL);
	}

	return S_OK;*/

	//Slow and safe, but still faster than SHGetPathFromIDList 

	CComPtr<IShellFolder> pDesktopFolder = NULL;

	HRESULT hr = SHGetDesktopFolder(&pDesktopFolder);

	if (!SUCCEEDED(hr))
		return hr;

	STRRET name;

	hr = pDesktopFolder->GetDisplayNameOf(pIDL, SHGDN_FORPARSING, &name);

	if (SUCCEEDED(hr))
	{
		WCHAR* sName = NULL;

		hr = StrRetToStr (&name, NULL, &sName);

		if (SUCCEEDED(hr))
		{
			if (sName != NULL)
			{
				rResult = sName;

				CoTaskMemFree(sName);
			}
			else hr = S_FALSE;
		}
	}

	return hr;
}



void		EnsureFullPath(std::wstring& rsFile)
{
	if (rsFile.find(L'\\') == -1 )
	{
		const DWORD nSize = ExpandEnvironmentStrings(L"%PATH%", NULL, 0);
		
		std::wstring sPathEnv;
		
		sPathEnv.resize(nSize);

		ExpandEnvironmentStrings(L"%PATH%", (LPWSTR)sPathEnv.c_str(), nSize);

		sPathEnv.resize(nSize-1);

		LPWSTR sTemp = (LPWSTR)sPathEnv.c_str();

		while(sTemp != NULL)
		{
			LPWSTR sNext = wcschr(sTemp,L';');

			if (sNext != NULL)
				*sNext++ = '\0';
			
			std::wstring sTest = sTemp;
			
			sTest += L"\\" + rsFile;

			if (GetFileAttributes(sTest.c_str()) != INVALID_FILE_ATTRIBUTES)
			{
				rsFile = sTest;
				break;
			}
			else sTemp = sNext;
		}
	}
}
