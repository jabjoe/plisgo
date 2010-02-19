/*
* Copyright (c) 2009, Eurocom Entertainment Software
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 	* Redistributions of source code must retain the above copyright
* 	  notice, this list of conditions and the following disclaimer.
* 	* Redistributions in binary form must reproduce the above copyright
* 	  notice, this list of conditions and the following disclaimer in the
* 	  documentation and/or other materials provided with the distribution.
* 	* Neither the name of the Eurocom Entertainment Software nor the
*	  names of its contributors may be used to endorse or promote products
* 	  derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY EUROCOM ENTERTAINMENT SOFTWARE ''AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* =============================================================================
*
* This is part of a Plisgo/Dokan example.
* Written by Joe Burmeister (plisgo@eurocom.co.uk)
*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <shlobj.h>
#include "../PlisgoFSLib/PlisgoFSHiddenFolders.h"
#include "resource.h"
#include "ProcessesFolder.h"
#include "Psapi.h"


class ProcessFile : public PlisgoFSStringReadOnly
{
public:
	ProcessFile(const PROCESSENTRY32& rPE) : PlisgoFSStringReadOnly(rPE.szExeFile)
	{
		SetVolatile(true);
		m_PE = rPE;
	}

	virtual bool			IsValid() const				{ return false; } //Force it not to use cached

	virtual DWORD			GetAttributes() const		{ return FILE_ATTRIBUTE_READONLY; }

	const PROCESSENTRY32&	GetProcessEntry32() const	{ return m_PE;}

private:
	PROCESSENTRY32 m_PE;
};




static void ProcessTerminator(uintptr_t nProcessID)
{
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, nProcessID);

	if (hProcess != NULL)
	{
		WCHAR sBuffer[MAX_PATH];

		wsprintf(sBuffer,L"Terminate process %i?", nProcessID);

		if (MessageBox(GetTopWindow(0), sBuffer, L"ProcessFS", MB_YESNO) == IDYES)
		{
			const BOOL bTerminated = TerminateProcess(hProcess, 0);

			if (bTerminated)
			{
				for(int n = 0; hProcess != NULL && n < 10; ++n)
				{
					CloseHandle(hProcess);
					hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, nProcessID);

					if (hProcess != NULL)
						Sleep(100);
				}
			}

			if (hProcess != NULL)
				CloseHandle(hProcess);

			if (!bTerminated)
				MessageBox(GetTopWindow(0), L"Can't Terminate Process", L"ProcessFS", MB_ICONERROR);
		}
	}
	else
	{
		WCHAR sBuffer[MAX_PATH];

		wsprintf(sBuffer,L"Can't Access Process %i", nProcessID);

		MessageBox(GetTopWindow(0), sBuffer, L"ProcessFS", MB_ICONERROR);
	}
}


static bool CanTerminateProcess(DWORD nProcessID)
{
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, nProcessID);

	if (hProcess == NULL)
		return false;

	CloseHandle(hProcess);

	return true;
}



class KillMenuItemClick : public StringEvent
{
public:

	virtual bool Do(LPCWSTR sPath)
	{
		if (sPath != NULL && sPath[0] != L'\0')
			ProcessTerminator(_wtoi(&sPath[1]));

		return true;
	}

private:

	ProcessesFolder*	m_pProcessesFolder;
};



class KillMenuItemEnable : public StringEvent
{
public:
	virtual bool Do(LPCWSTR sPath)
	{
		if (sPath == NULL)
			return true;

		if (sPath[0] == L'\0')
			return false;

		return CanTerminateProcess(_wtoi(&sPath[1]));
	}
};





static HWND		GetProcessWindow(DWORD nProcessID)
{
	HWND hWnd = ::GetTopWindow(0);

	while ( hWnd )
	{
		DWORD nOtherProcessId;
		DWORD nThreadId = ::GetWindowThreadProcessId( hWnd, &nOtherProcessId);

		if ( nOtherProcessId == nProcessID )
			return hWnd;
		
		hWnd = ::GetNextWindow( hWnd , GW_HWNDNEXT);
	}

	return NULL;
}



class SwitchToMenuItemClick : public StringEvent
{
public:

	virtual bool Do(LPCWSTR sPath)
	{
		if (sPath == NULL || sPath[0] == L'\0')
			return true;

		DWORD nProcess = _wtoi(&sPath[1]);

		HWND hWnd = GetProcessWindow(nProcess);

		if (hWnd != NULL)
		{
			hWnd = GetAncestor(hWnd, GA_ROOT);

			if (IsIconic(hWnd))
				ShowWindow(hWnd, SW_RESTORE);

			MessageBox(NULL, L"Love to know how this should be done,\nbecause none of the ways I found work 100%.", L"This should be simple! Not my fault!", MB_OK);

			AllowSetForegroundWindow(nProcess);
			BringWindowToTop(hWnd);
			SetForegroundWindow(hWnd);
			SwitchToThisWindow(hWnd, TRUE);
		}
		
		return true;
	}
};


class SwitchToMenuItemEnable : public StringEvent
{
public:

	virtual bool Do(LPCWSTR sPath)
	{
		if (sPath == NULL)
			return true;

		if (sPath[0] == L'\0')
			return false;

		return (GetProcessWindow(_wtoi(&sPath[1])) != NULL);
	}
};



static bool ResolveDevicePath(std::wstring& rsFile)
{ 
	WCHAR sDrives[MAX_PATH] = {0};
      
    if (!GetLogicalDriveStrings(MAX_PATH, sDrives))
		return false;
	
	WCHAR* sDrivesPos = sDrives;

	for(;*sDrivesPos;sDrivesPos += 4)
	{
		WCHAR sName[MAX_PATH];
		WCHAR sDrive[3] = L" :";

		sDrive[0] = *sDrivesPos;

		if (!QueryDosDevice(sDrive, sName, MAX_PATH))
			return false;

		size_t nNameLen = wcslen(sName);

		if (_wcsnicmp(rsFile.c_str(), sName, nNameLen) == 0)
		{
			rsFile.replace(0, nNameLen, sDrive);

			return true;
		}
	}

	return false;	
}


ProcessesFolderShellInterface::ProcessesFolderShellInterface()
{
	HMODULE hModule = GetModuleHandle(NULL);

	HRSRC hRes = FindResourceW(hModule, MAKEINTRESOURCEW(IDR_JPEG1), L"JPEG");

	if (hRes != NULL)
	{
		DWORD nDataSize = SizeofResource(hModule, hRes);

		HGLOBAL hReallyStaticMem = LoadResource(hModule, hRes);

		if (hReallyStaticMem != NULL)
		{
			BYTE* pData = (BYTE*)LockResource(hReallyStaticMem);

			if (pData != NULL)
				m_ThumbnailPlaceHolder.reset(new PlisgoFSDataFile( pData, nDataSize ));
		}
	}
}



IPtrRootPlisgoFSFolder ProcessesFolderShellInterface::CreatePlisgoFolder(const std::wstring& rsPath, IPtrPlisgoVFS& rVFS)
{
	IPtrRootPlisgoFSFolder plisgoRoot = IShellInfoFetcher::CreatePlisgoFolder(rsPath, rVFS);

	assert(plisgoRoot.get() != NULL);

	plisgoRoot->AddColumn(L"Exe File");
	plisgoRoot->SetColumnAlignment(0, RootPlisgoFSFolder::LEFT);
	plisgoRoot->SetColumnDefaultWidth(0, 20);
	plisgoRoot->AddColumn(L"Thread Num");
	plisgoRoot->SetColumnType(1, RootPlisgoFSFolder::INT);
	plisgoRoot->AddColumn(L"Parent Process");
	plisgoRoot->SetColumnType(2, RootPlisgoFSFolder::INT);
	plisgoRoot->AddColumn(L"Window Text");
	plisgoRoot->SetColumnDefaultWidth(3, 20);

	plisgoRoot->AddCustomDefaultIcon(0, 0);

	HINSTANCE hHandle = GetModuleHandle(NULL);

	plisgoRoot->AddPngIcons(hHandle, 0, MAKEINTRESOURCEW(IDB_ICON_16));
	plisgoRoot->AddPngIcons(hHandle, 0, MAKEINTRESOURCEW(IDB_ICON_32));
	plisgoRoot->AddPngIcons(hHandle, 1, MAKEINTRESOURCEW(IDB_LOCK16));
	plisgoRoot->AddPngIcons(hHandle, 1, MAKEINTRESOURCEW(IDB_LOCK32));
	plisgoRoot->AddPngIcons(hHandle, 1, MAKEINTRESOURCEW(IDB_LOCK96));

	plisgoRoot->EnableCustomIcons();
	plisgoRoot->EnableOverlays();
	plisgoRoot->EnableThumbnails();

	plisgoRoot->DisableStandardColumn(RootPlisgoFSFolder::StdColumn_Type);
	plisgoRoot->DisableStandardColumn(RootPlisgoFSFolder::StdColumn_Date_Mod);
	plisgoRoot->DisableStandardColumn(RootPlisgoFSFolder::StdColumn_Date_Crt);
	plisgoRoot->DisableStandardColumn(RootPlisgoFSFolder::StdColumn_Date_Acc);

	int nMenu = plisgoRoot->AddMenu(L"Processes", IPtrStringEvent(), -1, IPtrStringEvent(), 0, 0);

	plisgoRoot->AddMenu(L"Kill Selected", IPtrStringEvent(new KillMenuItemClick()), nMenu, IPtrStringEvent(new KillMenuItemEnable()), 0, 0);
	plisgoRoot->AddMenu(L"Switch to", IPtrStringEvent(new SwitchToMenuItemClick()), nMenu, IPtrStringEvent(new SwitchToMenuItemEnable()), 0, 0);

	return plisgoRoot;
}


bool	ProcessesFolderShellInterface::IsShelled(IPtrPlisgoFSFile& rFile) const
{
	return (dynamic_cast<ProcessesFolder*>(rFile.get()) != NULL);
}


bool	ProcessesFolderShellInterface::GetColumnEntry(IPtrPlisgoFSFile& rFile, const int nColumnIndex, std::wstring& rsResult) const
{
	ProcessFile* pProcessFile = dynamic_cast<ProcessFile*>(rFile.get());

	if (pProcessFile == NULL)
		return false;

	const PROCESSENTRY32& rEntry = pProcessFile->GetProcessEntry32();

	switch(nColumnIndex)
	{
	case 0:
		rsResult = rEntry.szExeFile;
		return true;
	case 1:
		rsResult = (boost::wformat(L"%1%") %rEntry.cntThreads).str();
		return true;
	case 2:
		rsResult = (boost::wformat(L"%1%") %rEntry.th32ParentProcessID).str();
		return true;
	case 3:
		{
			HWND hWnd = GetProcessWindow(rEntry.th32ProcessID);

			if (hWnd == NULL)
				return false;

			WCHAR sBuffer[MAX_PATH];

			if (GetWindowTextW(hWnd, sBuffer, MAX_PATH) == 0)
				return false;

			rsResult = sBuffer;

			return true;
		}

	default:
		return false;
	}
}


bool	ProcessesFolderShellInterface::GetOverlayIcon(IPtrPlisgoFSFile& rFile, IconLocation& rResult) const
{
	ProcessFile* pProcessFile = dynamic_cast<ProcessFile*>(rFile.get());

	if (pProcessFile == NULL)
		return false;

	if (CanTerminateProcess(pProcessFile->GetProcessEntry32().th32ProcessID))
		return false;

	rResult.Set(1,0);

	return true;
}


static BOOL CALLBACK	FoundCB(  HMODULE , LPCTSTR , LPTSTR , LONG_PTR lParam)
{
	*((bool*)lParam) = true;

	return FALSE;
}


bool	ProcessesFolderShellInterface::GetCustomIcon(IPtrPlisgoFSFile& rFile, IconLocation& rResult) const
{
	ProcessFile* pProcessFile = dynamic_cast<ProcessFile*>(rFile.get());

	if (pProcessFile == NULL)
		return false;

	const DWORD nProcessID = pProcessFile->GetProcessEntry32().th32ProcessID;

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, 0, nProcessID); 

	if (hProcess == NULL)
		return false;

	std::wstring sFile(MAX_PATH+1,L' ');

	DWORD nLength;

	while (nLength = GetProcessImageFileName(hProcess, (LPWSTR)sFile.c_str(), sFile.length()))
	{
		if (nLength == 0)
		{
			DWORD nError = GetLastError();

			if (nError == 0)
				break;
			
			if (nError != ERROR_INSUFFICIENT_BUFFER)
			{
				nLength = 0;
				break;
			}
			else sFile.resize(sFile.size()*2);
		}
		else break;
	}

	if (nLength)
	{
		sFile.resize(nLength);

		if (ResolveDevicePath(sFile))
		{
			HMODULE hModule = LoadLibraryEx(sFile.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE);

			if (hModule != NULL)
			{
				bool bFound = false;

				EnumResourceNames(hModule, RT_GROUP_ICON, FoundCB, (LONG_PTR)&bFound);

				FreeLibrary(hModule);

				if (bFound)
					rResult.Set(sFile,0);
				else
					nLength = 0;
			}
			else nLength = 0;
		}
		else nLength = 0;
	}

	CloseHandle(hProcess);

	return (nLength != 0);
}


bool	ProcessesFolderShellInterface::GetThumbnail(IPtrPlisgoFSFile& rFile, std::wstring& rsExt, IPtrPlisgoFSFile& rThumbnailFile) const
{
	if (m_ThumbnailPlaceHolder.get() == NULL)
		return false;

	if (dynamic_cast<ProcessFile*>(rFile.get()) == NULL)
		return false;

	rsExt = L".jpg";

	rThumbnailFile = m_ThumbnailPlaceHolder;

	return true;
}






ProcessesFolder::ProcessesFolder()
{
	//Add stubs for mounting of Plisgo GUI files
	m_Extras.AddFile(L".plisgofs", IPtrPlisgoFSFile(new PlisgoFSStringReadOnly()));
	m_Extras.AddFile(L"Desktop.ini", IPtrPlisgoFSFile(new PlisgoFSStringReadOnly()));
}


bool				ProcessesFolder::ForEachChild(EachChild& rEachChild) const
{
	if (!m_Extras.ForEachFile(rEachChild))
		return false;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

	if (hSnapshot != NULL)
	{
		PROCESSENTRY32 pe;

		// fill up its size
		pe.dwSize=sizeof(PROCESSENTRY32);

		if(Process32First(hSnapshot,&pe))
		{
			do
			{
				WCHAR sName[MAX_PATH];

				wsprintf(sName,L"%i", pe.th32ProcessID);

				IPtrPlisgoFSFile file = boost::make_shared<ProcessFile>(pe);

				if (!rEachChild.Do(sName, file))
					return false;

				pe.dwSize=sizeof(PROCESSENTRY32);
			}
			while(Process32Next(hSnapshot,&pe));   
		}

		CloseHandle(hSnapshot);
	}

	return true;
}


IPtrPlisgoFSFile	ProcessesFolder::GetChild(LPCWSTR sName) const
{
	IPtrPlisgoFSFile result = m_Extras.GetFile(sName);

	if (result.get() != NULL)
		return result;

	assert(sName != NULL);

	if (!isdigit(sName[0]))
		return IPtrPlisgoFSFile();

	const DWORD nProcessID = _wtoi(sName);

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

	if (hSnapshot != NULL)
	{
		PROCESSENTRY32 pe;

		// fill up its size
		pe.dwSize=sizeof(PROCESSENTRY32);

		if(Process32First(hSnapshot,&pe))
		{
			do
			{
				if (pe.th32ProcessID == nProcessID)
				{
					result = boost::make_shared<ProcessFile>(pe);

					break;
				}

				pe.dwSize=sizeof(PROCESSENTRY32);
			}
			while(Process32Next(hSnapshot,&pe));   
		}

		CloseHandle(hSnapshot);
	}

	return result;
}