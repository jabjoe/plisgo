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

#include "Utils.h"
#include "PathUtils.h"



bool		ExtIsCodeImage(LPCWSTR sExt)
{
	return (sExt != NULL && sExt[0] == L'.' &&
		((tolower(sExt[1]) == L'e' && tolower(sExt[2]) == L'x' && tolower(sExt[3]) == L'e') ||
		(tolower(sExt[1]) == L'd' && tolower(sExt[2]) == L'l' && tolower(sExt[3]) == L'l')));
}


bool		ExtIsShortcut(LPCWSTR sExt)
{
	return (sExt != NULL && sExt[0] != L'\0' &&
		(tolower(sExt[1]) == L'l' && tolower(sExt[2]) == L'n' && tolower(sExt[3]) == L'k'));
}


bool		ExtIsShortcutUrl(LPCWSTR sExt)
{
	return (sExt != NULL && sExt[0] != L'\0' &&
		(tolower(sExt[1]) == L'u' && tolower(sExt[2]) == L'r' && tolower(sExt[3]) == L'l'));
}


bool		ExtIsIconFile(LPCWSTR sExt)
{
	return (sExt != NULL && sExt[0] != L'\0' &&
		(tolower(sExt[1]) == L'i' && tolower(sExt[2]) == L'c' && tolower(sExt[3]) == L'o'));
}


static void AppendANSI(std::wstring& rResult, const char* sStr)
{
	int nAnsiLen = (int)strlen(sStr);

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
			while(*sTemp == L';')
				++sTemp;

			LPWSTR sNext = wcschr(sTemp,L';');

			std::wstring sTest;

			if (sNext != NULL)
				sTest.assign(sTemp, sNext-sTemp);
			else
				sTest.assign(sTemp);
			
			if (sTest.length())
			{
				sTest += L"\\" + rsFile;

				if (GetFileAttributes(sTest.c_str()) != INVALID_FILE_ATTRIBUTES)
				{
					rsFile = sTest;
					break;
				}
				else sTemp = sNext;
			}
			else sTemp = sNext;
		}
	}
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
			DWORD nWRead = MultiByteToWideChar(CP_UTF8, 0, buffer, (int)nRead, NULL, 0);

			WCHAR* wBuffer = (WCHAR*)_malloca(sizeof(WCHAR)*(nWRead+1));

			nWRead = MultiByteToWideChar(CP_UTF8, 0, buffer, (int)nRead, wBuffer, nWRead);

			rsResult.append(wBuffer, nWRead);
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


bool	GetFileVersion(	LPCWSTR sFile,
						WORD& rnMajor, WORD& rnMinor, 
						WORD& rnBugfix, WORD& rnBuild)
{
	LPBYTE	lpVersionInfo = NULL;
	bool	bResult = false;
	DWORD	dwDummy;
	DWORD	dwFVISize = GetFileVersionInfoSize( sFile , &dwDummy );

	if (dwFVISize == 0)
		return false;
	
	lpVersionInfo = (LPBYTE)malloc(dwFVISize);

	if (lpVersionInfo == NULL)
		return false;
		
	if (GetFileVersionInfo( sFile , 0 , dwFVISize , lpVersionInfo ))
	{
		UINT uLen;
		VS_FIXEDFILEINFO *lpFfi;
		VerQueryValue( lpVersionInfo , L"\\" , (LPVOID *)&lpFfi , &uLen );

		rnMajor = HIWORD(lpFfi->dwFileVersionMS);
		rnMinor = LOWORD(lpFfi->dwFileVersionMS);
		rnBugfix = HIWORD(lpFfi->dwFileVersionLS);
		rnBuild	= LOWORD(lpFfi->dwFileVersionLS);

		bResult = true;
	}

	free(lpVersionInfo);

	return bResult;
}