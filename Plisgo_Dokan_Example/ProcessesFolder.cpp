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


class ProcessFile : public PlisgoFSStringFile
{
public:
	ProcessFile(const PROCESSENTRY32& rPE) : PlisgoFSStringFile(rPE.szExeFile, true)
	{
	}

	virtual bool	IsValid() const			{ return false; } //Force it not to use cached

	virtual DWORD	GetAttributes() const	{ return FILE_ATTRIBUTE_READONLY; }

};


struct ProcessTerminatorPacket
{
	DWORD				nProcessID;
	ProcessesFolder*	pProcessesFolder;
};



DWORD WINAPI ProcessTerminator(ProcessTerminatorPacket* pPacket)
{
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pPacket->nProcessID);

	if (hProcess != NULL)
	{
		WCHAR sBuffer[MAX_PATH];

		wsprintf(sBuffer,L"Terminate process %i?", pPacket->nProcessID);

		if (MessageBox(GetTopWindow(0), sBuffer, L"ProcessFS", MB_YESNO) == IDYES)
		{
			const BOOL bTerminated = TerminateProcess(hProcess, 0);

			if (bTerminated)
			{
				for(int n = 0; hProcess != NULL && n < 10; ++n)
				{
					CloseHandle(hProcess);
					hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pPacket->nProcessID);

					if (hProcess != NULL)
						Sleep(100);
				}
			}

			if (hProcess != NULL)
				CloseHandle(hProcess);

			if (bTerminated)
			{
				pPacket->pProcessesFolder->RefreshProcesses();

				SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, L"X:\\processes\\", NULL);
			}
			else MessageBox(GetTopWindow(0), L"Can't Terminate Process", L"ProcessFS", MB_ICONERROR);
		}
	}
	else
	{
		WCHAR sBuffer[MAX_PATH];

		wsprintf(sBuffer,L"Can't Access Process %i", pPacket->nProcessID);

		MessageBox(GetTopWindow(0), sBuffer, L"ProcessFS", MB_ICONERROR);
	}

	delete pPacket;

	return 0;
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

	KillMenuItemClick(ProcessesFolder* pProcessesFolder)
	{
		m_pProcessesFolder = pProcessesFolder;
	}


	virtual bool Do(LPCWSTR sPath)
	{
		if (sPath != NULL && sPath[0] != L'\0')
		{
			ProcessTerminatorPacket* pPacket = new ProcessTerminatorPacket();

			pPacket->nProcessID			= _wtoi(&sPath[1]);
			pPacket->pProcessesFolder	= m_pProcessesFolder;

			if (CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ProcessTerminator, pPacket, 0, NULL) == NULL)
				delete pPacket;
		}

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








ProcessesFolder::ProcessesFolder()
{
	IPtrRootPlisgoFSFolder plisgoRoot(new RootPlisgoFSFolder(L"processesFS", this));

	assert(plisgoRoot.get() != NULL);

	m_Extras.AddFile(L".plisgofs", plisgoRoot);
	m_Extras.AddFile(L"Desktop.ini", GetPlisgoDesktopIniFile());

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

	plisgoRoot->AddMenu(L"Kill Selected", IPtrStringEvent(new KillMenuItemClick(this)), nMenu, IPtrStringEvent(new KillMenuItemEnable()), 0, 0);
	plisgoRoot->AddMenu(L"Switch to", IPtrStringEvent(new SwitchToMenuItemClick()), nMenu, IPtrStringEvent(new SwitchToMenuItemEnable()), 0, 0);	

	LoadDriveDevices();

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

	Update();
}

void				ProcessesFolder::RefreshProcesses()
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	Update();
}


// PlisgoFSFolder interface

bool				ProcessesFolder::ForEachChild(EachChild& rEachChild) const
{
	if (!m_Extras.ForEachFile(rEachChild))
		return false;

	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	{
		boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

		const_cast<ProcessesFolder*>(this)->Update();
	}

	for(std::map<DWORD, PROCESSENTRY32>::const_iterator it = m_Processes.begin();
		it != m_Processes.end(); ++it)
	{
		IPtrPlisgoFSFile file = GetProcessFile(it->first);

		if (file.get() != NULL)
		{
			WCHAR sName[MAX_PATH];

			wsprintf(sName,L"%i", it->first);

			if (!rEachChild.Do(sName, file))
				return false;
		}
	}

	return true;
}


IPtrPlisgoFSFile	ProcessesFolder::GetChild(LPCWSTR sName) const
{
	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	IPtrPlisgoFSFile result = m_Extras.GetFile(sName);

	if (result.get() != NULL)
		return result;

	assert(sName != NULL);

	if (!isdigit(sName[0]))
		return IPtrPlisgoFSFile();

	const DWORD nProcessID = _wtoi(sName);

	return GetProcessFile(nProcessID);
}




//IShellInfoFetcher Interface


bool				ProcessesFolder::ReadShelled(const std::wstring& rsFolderPath, IShellInfoFetcher::BasicFolder* pResult) const
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	if (pResult == NULL)
	{
		if (rsFolderPath.length() > 1)
			return false;

		if (rsFolderPath.length() == 0)
			return true;

		return (rsFolderPath[0] == L'\\');
	}


	for(std::map<DWORD, PROCESSENTRY32>::const_iterator it = m_Processes.begin(); it != m_Processes.end(); ++it)
	{
		pResult->resize(pResult->size()+1);

		IShellInfoFetcher::BasicFileInfo& rFile = pResult->back();

		rFile.bIsFolder = false;
		wcscpy_s(rFile.sName, MAX_PATH, (boost::wformat(L"%1%") %it->first).str().c_str());
	}

	return true;
}


bool				ProcessesFolder::GetColumnEntry(const std::wstring& rsFilePath, const int nColumnIndex, std::wstring& rsResult) const
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	assert(rsFilePath.length() > 1);

	const DWORD nProcessID = _wtoi(&rsFilePath.c_str()[1]); //Skip slash

	std::map<DWORD, PROCESSENTRY32>::const_iterator it = m_Processes.find(nProcessID);

	if (it == m_Processes.end())
		return false;

	switch(nColumnIndex)
	{
	case 0:
		rsResult = it->second.szExeFile;
		return true;
	case 1:
		rsResult = (boost::wformat(L"%1%") %it->second.cntThreads).str();
		return true;
	case 2:
		rsResult = (boost::wformat(L"%1%") %it->second.th32ParentProcessID).str();
		return true;
	case 3:
		{
			HWND hWnd = GetProcessWindow(nProcessID);

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


bool				ProcessesFolder::GetOverlayIcon(const std::wstring& rsFilePath, IconLocation& rResult) const
{
	if (rsFilePath.length() < 2)
		return false;

	if (CanTerminateProcess(_wtoi(&rsFilePath[1])))
		return false;

	rResult.Set(1,0);

	return true;
}


void				ProcessesFolder::LoadDriveDevices()
{
	m_DriveDevices.clear();

	WCHAR sDrives[MAX_PATH];

	DWORD nDriveLen = GetLogicalDriveStrings(MAX_PATH, sDrives);

	for(WCHAR* sDrive = sDrives; sDrive < &sDrives[nDriveLen];)
	{
		WCHAR sBuffer[MAX_PATH];

		if (sDrive[2] == L'\\')
			sDrive[2] = L'\0';

		if (QueryDosDevice(sDrive, sBuffer, MAX_PATH))
			m_DriveDevices.push_back(DriveEntry(sDrive, sBuffer));

		sDrive += 4;
	}
}


bool				ProcessesFolder::ResolveDevicePath(std::wstring& rsPath) const
{
	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	for(int n = 0; n < 2; ++n)
	{
		for(std::vector<DriveEntry>::const_iterator it = m_DriveDevices.begin(); it != m_DriveDevices.end(); ++it)
		{
			if (rsPath.find(it->second) == 0)
			{
				rsPath.replace(0, it->second.length(), it->first);

				return true;
			}
		}

		boost::upgrade_to_unique_lock<boost::shared_mutex> rwLock(lock);

		const_cast<ProcessesFolder*>(this)->LoadDriveDevices();
	}

	return false;
}


BOOL CALLBACK	FoundCB(  HMODULE , LPCTSTR , LPTSTR , LONG_PTR lParam)
{
	*((bool*)lParam) = true;

	return FALSE;
}



bool				ProcessesFolder::GetCustomIcon(const std::wstring& rsFilePath, IconLocation& rResult) const
{
	if (rsFilePath.length() < 2)
		return false;

	if (!isdigit(rsFilePath[1]))
		return false;

	const DWORD nProcessID = _wtoi(&rsFilePath.c_str()[1]); //Skip slash

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


bool				ProcessesFolder::GetThumbnail(const std::wstring& rsFilePath, LPCWSTR sExt, IPtrPlisgoFSFile& rThumbnailFile) const
{
	if (m_ThumbnailPlaceHolder.get() == NULL || sExt == NULL)
		return false;

	if (!MatchLower(sExt, L".jpg"))
		return false;
	
	rThumbnailFile = m_ThumbnailPlaceHolder;

	return true;
}


void				ProcessesFolder::Update()
{
	m_Processes.clear();

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
				m_Processes[pe.th32ProcessID] = pe;

				pe.dwSize=sizeof(PROCESSENTRY32);
			}
			while(Process32Next(hSnapshot,&pe));   
		}

		CloseHandle(hSnapshot);
	}
}


IPtrPlisgoFSFile	ProcessesFolder::GetProcessFile(const DWORD nProcessID) const
{
	std::map<DWORD, PROCESSENTRY32>::const_iterator it = m_Processes.find(nProcessID);

	if (it == m_Processes.end())
		return IPtrPlisgoFSFile();

	return boost::make_shared<ProcessFile>(it->second);
}
