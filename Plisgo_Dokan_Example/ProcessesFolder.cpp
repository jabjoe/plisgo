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

#pragma warning(disable:4996)

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <shlobj.h>
#include <Ntsecapi.h>
#include "../PlisgoFSLib/PlisgoFSHiddenFolders.h"
#include "resource.h"
#include "ProcessesFolder.h"
#include "Psapi.h"


class ProcessFolder : public PlisgoFSFolder
{
public:
	ProcessFolder(const PROCESSENTRY32& rPE);

	virtual bool				IsValid() const				{ return false; } //Force it not to use cached

	virtual DWORD				GetAttributes() const		{ return FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_DIRECTORY; }

	const PROCESSENTRY32&		GetProcessEntry32() const	{ return m_PE;}

	virtual int					GetChildren(ChildNames& rChildren) const	{ return m_ProcessInfoFile.GetFileNames(rChildren); }
	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sName) const				{ return m_ProcessInfoFile.GetFile(sName); }

private:

	PROCESSENTRY32					m_PE;
	
	mutable boost::shared_mutex		m_Mutex;
	PlisgoFSFileMap					m_ProcessInfoFile;

};


typedef NTSTATUS (WINAPI *NtQueryInformationProcessCB)(
  __in       HANDLE ProcessHandle,
  __in       PBYTE ProcessInformationClass,
  __out      PVOID ProcessInformation,
  __in       ULONG ProcessInformationLength,
  __out_opt  PULONG ReturnLength
);

NtQueryInformationProcessCB NtQueryInformationProcess = NULL;

HMODULE	hNTDLL_Lib = NULL;

typedef struct _PROCESS_BASIC_INFORMATION {
    PVOID Reserved1;
    PBYTE PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    PVOID Reserved3;
} PROCESS_BASIC_INFORMATION;


#define PROC_PARAMS_OFFSET		16
#define	CMDLINE_LENGTH_OFFSET	64


IPtrPlisgoFSFile	CreateArgsFile(HANDLE hProcess)
{
	if (NtQueryInformationProcess == NULL)
		return IPtrPlisgoFSFile();
			
	IPtrPlisgoFSFile result;

	PROCESS_BASIC_INFORMATION info;

	if (SUCCEEDED(NtQueryInformationProcess(hProcess, 0/*ProcessBasicInformation*/, &info, sizeof(PROCESS_BASIC_INFORMATION), NULL)))
	{
		ULONG32 nPorcessParamaters;

		//Get address of ProcessParameters of PEB structure
		if (ReadProcessMemory(hProcess, (LPVOID)(info.PebBaseAddress + PROC_PARAMS_OFFSET), &nPorcessParamaters, sizeof(ULONG32), NULL))
		{
			UNICODE_STRING cmdLine;

			if (ReadProcessMemory(hProcess, (LPVOID)(nPorcessParamaters + CMDLINE_LENGTH_OFFSET), &cmdLine, sizeof(UNICODE_STRING), NULL))
			{
				WCHAR* pszCmdLine = new WCHAR[cmdLine.MaximumLength];

				assert(pszCmdLine != NULL);

				if (ReadProcessMemory(hProcess, (LPVOID)cmdLine.Buffer, pszCmdLine, cmdLine.MaximumLength, NULL))
					result = IPtrPlisgoFSFile(new PlisgoFSStringReadOnly(pszCmdLine));

				delete[] pszCmdLine;
			}
		}
	}

	return result;
}



void				InitAddProcessFiles(PlisgoFSFileMap& rFiles, DWORD nPID)
{
	HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, nPID);

	MODULEENTRY32 me32;

	me32.dwSize = sizeof( MODULEENTRY32 );

	if( Module32First( hModuleSnap, &me32 ) )
	{
		rFiles[L"exe"] = IPtrPlisgoFSFile(new PlisgoFSStringReadOnly(me32.szExePath));

		CloseHandle(hModuleSnap);
	}
	else rFiles[L"exe"] = IPtrPlisgoFSFile(new PlisgoFSStringReadOnly(L"access_denied"));


	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, nPID);

	if (hProcess != NULL || hProcess != INVALID_HANDLE_VALUE)
	{
		rFiles[L"args"] = CreateArgsFile(hProcess);

		CloseHandle(hProcess);
	}
	else
	{
		rFiles[L"args"] = IPtrPlisgoFSFile(new PlisgoFSStringReadOnly(L"access_denied"));
	}	
}






ProcessFolder::ProcessFolder(const PROCESSENTRY32& rPE)
{
	m_PE = rPE;

	InitAddProcessFiles(m_ProcessInfoFile, rPE.th32ProcessID);
}



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



class KillMenuItemClick : public FileEvent
{
public:

	virtual bool Do(IPtrPlisgoFSFile& rFile)
	{
		if (rFile.get() == NULL)
			return true;

		ProcessFolder* pProcessFolder = dynamic_cast<ProcessFolder*>(rFile.get());

		if (pProcessFolder == NULL)
			return true;

		DWORD nProcess = pProcessFolder->GetProcessEntry32().th32ProcessID;

		ProcessTerminator(nProcess);

		return true;
	}

private:

	ProcessesFolder*	m_pProcessesFolder;
};



class KillMenuItemEnable : public FileEvent
{
public:
	virtual bool Do(IPtrPlisgoFSFile& rFile)
	{
		if (rFile.get() == NULL)
			return true;

		ProcessFolder* pProcessFolder = dynamic_cast<ProcessFolder*>(rFile.get());

		if (pProcessFolder == NULL)
			return true;

		DWORD nProcess = pProcessFolder->GetProcessEntry32().th32ProcessID;

		return CanTerminateProcess(nProcess);
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



class SwitchToMenuItemClick : public FileEvent
{
public:

	virtual bool Do(IPtrPlisgoFSFile& rFile)
	{
		if (rFile.get() == NULL)
			return true;

		ProcessFolder* pProcessFolder = dynamic_cast<ProcessFolder*>(rFile.get());

		if (pProcessFolder == NULL)
			return true;

		DWORD nProcess = pProcessFolder->GetProcessEntry32().th32ProcessID;

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


class SwitchToMenuItemEnable : public FileEvent
{
public:

	virtual bool Do(IPtrPlisgoFSFile& rFile)
	{
		if (rFile.get() == NULL)
			return true;

		ProcessFolder* pProcessFolder = dynamic_cast<ProcessFolder*>(rFile.get());

		if (pProcessFolder == NULL)
			return true;

		return (GetProcessWindow(pProcessFolder->GetProcessEntry32().th32ProcessID) != NULL);
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



IPtrRootPlisgoFSFolder ProcessesFolderShellInterface::CreatePlisgoFolder(IPtrPlisgoVFS& rVFS, const std::wstring& rsPath, bool bDoMounts)
{
	IPtrRootPlisgoFSFolder plisgoRoot = IShellInfoFetcher::CreatePlisgoFolder(rVFS, rsPath, bDoMounts);

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
	plisgoRoot->AddCustomFolderIcon(0, 0, 0, 0);

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

	int nMenu = plisgoRoot->AddMenu(L"Processes", IPtrFileEvent(), -1, 0, 0);

	plisgoRoot->AddMenu(L"Kill Selected", IPtrFileEvent(new KillMenuItemClick()), nMenu, 0, 0, IPtrFileEvent(new KillMenuItemEnable()));
	plisgoRoot->AddMenu(L"Switch to", IPtrFileEvent(new SwitchToMenuItemClick()), nMenu, 0, 0, IPtrFileEvent(new SwitchToMenuItemEnable()));

	return plisgoRoot;
}


bool	ProcessesFolderShellInterface::GetColumnEntry(IPtrPlisgoFSFile& rFile, const int nColumnIndex, std::wstring& rsResult) const
{
	ProcessFolder* pProcessFolder = dynamic_cast<ProcessFolder*>(rFile.get());

	if (pProcessFolder == NULL)
		return false;

	const PROCESSENTRY32& rEntry = pProcessFolder->GetProcessEntry32();

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
	ProcessFolder* pProcessFolder = dynamic_cast<ProcessFolder*>(rFile.get());

	if (pProcessFolder == NULL)
	{
		if (dynamic_cast<PlisgoFSStringReadOnly*>(rFile.get()) != NULL)
		{
			rResult.Set(1,0);
			return true;
		}	

		return false;
	}

	if (CanTerminateProcess(pProcessFolder->GetProcessEntry32().th32ProcessID))
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
	ProcessFolder* pProcessFolder = dynamic_cast<ProcessFolder*>(rFile.get());

	if (pProcessFolder == NULL)
		return false;

	const DWORD nProcessID = pProcessFolder->GetProcessEntry32().th32ProcessID;

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

	if (dynamic_cast<ProcessFolder*>(rFile.get()) == NULL)
		return false;

	rsExt = L".jpg";

	rThumbnailFile = m_ThumbnailPlaceHolder;

	return true;
}






ProcessesFolder::ProcessesFolder()
{
	//Add stubs for mounting of Plisgo GUI files
	m_Extras[L".plisgofs"] = IPtrPlisgoFSFile(new PlisgoFSStringReadOnly());
	m_Extras[L"Desktop.ini"] = IPtrPlisgoFSFile(new PlisgoFSStringReadOnly());

	m_nProcessSnapTime = 0;

	hNTDLL_Lib = LoadLibrary(L"Ntdll.dll");

	if (hNTDLL_Lib != NULL && hNTDLL_Lib != INVALID_HANDLE_VALUE)
		NtQueryInformationProcess = (NtQueryInformationProcessCB)GetProcAddress(hNTDLL_Lib, "NtQueryInformationProcess");
}

ProcessesFolder::~ProcessesFolder()
{
	if (hNTDLL_Lib != NULL && hNTDLL_Lib != INVALID_HANDLE_VALUE)
	{
		FreeLibrary(hNTDLL_Lib);
		hNTDLL_Lib = NULL;
	}
}


int				ProcessesFolder::GetChildren(ChildNames& rChildren) const
{
	int nError = m_Extras.GetFileNames(rChildren);

	if (nError != 0)
		return nError;

	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	if (IsStale())
	{
		boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

		const_cast<ProcessesFolder*>(this)->LoadProcesses();
	}

	return m_Processes.GetFileNames(rChildren);
}


IPtrPlisgoFSFile	ProcessesFolder::GetChild(LPCWSTR sName) const
{
	IPtrPlisgoFSFile result = m_Extras.GetFile(sName);

	if (result.get() != NULL)
		return result;

	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	if (IsStale())
	{
		boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

		const_cast<ProcessesFolder*>(this)->LoadProcesses();
	}

	return m_Processes.GetFile(sName);
}


bool				ProcessesFolder::IsStale() const
{
	ULONG64	nNow = 0;

	GetSystemTimeAsFileTime((FILETIME*)&nNow);

	ULONG64 nDeltaSecs = (nNow-m_nProcessSnapTime)/10000000 /*NTSECOND*/;

	return (nDeltaSecs > 5);
}


void				ProcessesFolder::LoadProcesses()
{
	m_Processes.clear();

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

	if (hSnapshot != NULL)
	{
		PROCESSENTRY32 pe;
				
		WCHAR sName[MAX_PATH];

		// fill up its size
		pe.dwSize=sizeof(PROCESSENTRY32);

		if(Process32First(hSnapshot,&pe))
		{
			do
			{
				wsprintf(sName,L"%i", pe.th32ProcessID);

				m_Processes[sName] = boost::make_shared<ProcessFolder>(pe);

				pe.dwSize=sizeof(PROCESSENTRY32);
			}
			while(Process32Next(hSnapshot,&pe));   
		}

		CloseHandle(hSnapshot);
	}

	GetSystemTimeAsFileTime((FILETIME*)&m_nProcessSnapTime);
}
