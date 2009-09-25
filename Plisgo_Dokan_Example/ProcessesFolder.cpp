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
#include "PlisgoFSHiddenFolders.h"
#include "resource.h"
#include "ProcessesFolder.h"
#include "Psapi.h"


class ProcessFile : public PlisgoFSStringFile
{
public:
	ProcessFile(const PROCESSENTRY32& rPE) : PlisgoFSStringFile((boost::wformat(L"\\%1%") %rPE.th32ProcessID).str(), rPE.szExeFile, true)
	{
	}

	virtual DWORD	GetAttributes() const		{ return FILE_ATTRIBUTE_READONLY; }
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

		HWND hWnd = GetProcessWindow(_wtoi(&sPath[1]));

		if (hWnd != NULL)
		{
			hWnd = GetAncestor(hWnd, GA_ROOT);

			ShowWindow(hWnd, SW_SHOWNORMAL);

			SwitchToThisWindow(hWnd, FALSE);
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
	m_sPath = L"\\processes";

	IPtrRootPlisgoFSFolder plisgoRoot(new RootPlisgoFSFolder(L"processesFS", this));

	assert(plisgoRoot.get() != NULL);

	m_Extras.AddFile(plisgoRoot);
	m_Extras.AddFile(GetPlisgoDesktopIniFile(L"\\processes"));

	plisgoRoot->AddColumn(L"Exe File");
	plisgoRoot->SetColumnAlignment(0, RootPlisgoFSFolder::LEFT);
	plisgoRoot->AddColumn(L"Thread Num");
	plisgoRoot->SetColumnType(1, RootPlisgoFSFolder::INT);
	plisgoRoot->AddColumn(L"Parent Process");
	plisgoRoot->SetColumnType(2, RootPlisgoFSFolder::INT);
	plisgoRoot->AddColumn(L"Window Text");

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
				m_ThumbnailPlaceHolder.reset(new PlisgoFSDataFile( L"N/A", pData, nDataSize ));
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


IPtrPlisgoFSFile	ProcessesFolder::GetDescendant(LPCWSTR sPath) const
{
	assert(sPath != NULL);

	if (sPath[0] == L'\0')
		return IPtrPlisgoFSFile();

	if (sPath[0] == L'\\')
		++sPath;

	IPtrPlisgoFSFile result = m_Extras.ParseName(sPath);

	if (result.get() != NULL)
	{
		if (sPath[0] == L'\0')
			return result;

		if (result->GetAsFolder() == NULL || sPath[0] != L'\\')
			return IPtrPlisgoFSFile();

		++sPath;

		if (sPath[0] == L'\0')
			return result;

		return result->GetAsFolder()->GetDescendant(sPath);
	}

	return GetChild(sPath);
}


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
			if (!rEachChild.Do(file))
				return false;
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


UINT				ProcessesFolder::GetChildNum() const
{ 
	boost::upgrade_lock<boost::shared_mutex> lock(m_Mutex);

	return (UINT)(m_Processes.size() + m_Extras.GetLength());
} 




//IShellInfoFetcher Interface

bool				ProcessesFolder::IsShelledFolder(const std::wstring& rsFolderPath) const
{
	if (rsFolderPath.length() > 1)
		return false;

	if (rsFolderPath.length() == 0)
		return true;

	return (rsFolderPath[0] == L'\\');
}


bool				ProcessesFolder::ReadShelled(const std::wstring& rsFolderPath, IShellInfoFetcher::BasicFolder& rResult) const
{
	boost::shared_lock<boost::shared_mutex> lock(m_Mutex);

	if (!IsShelledFolder(rsFolderPath))
		return false;

	for(std::map<DWORD, PROCESSENTRY32>::const_iterator it = m_Processes.begin(); it != m_Processes.end(); ++it)
	{
		rResult.resize(rResult.size()+1);

		IShellInfoFetcher::BasicFileInfo& rFile = rResult.back();

		rFile.bIsFolder = false;
		rFile.sName = (boost::wformat(L"%1%") %it->first).str();
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
		DWORD nError = GetLastError();

		if (nError == 0)
			break;
		
		if (nError != ERROR_INSUFFICIENT_BUFFER)
		{
			nLength = 0;
			break;
		}
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

bool				ProcessesFolder::GetThumbnail(const std::wstring& rsFilePath, const std::wstring& rsVirturalPath, IPtrPlisgoFSFile& rThumbnailFile) const
{
	if (m_ThumbnailPlaceHolder.get() == NULL)
		return false;

	size_t nExt = rsVirturalPath.find_last_of(L'.');

	if (nExt == -1)
		return false;

	if (rsVirturalPath.length()-nExt == 4)
	{
		if (tolower(rsVirturalPath[nExt+1]) != L'j' ||
			tolower(rsVirturalPath[nExt+2]) != L'p' ||
			tolower(rsVirturalPath[nExt+3]) != L'g')
			return false;
	}
	else if (rsVirturalPath.length()-nExt == 5)
	{
		if (tolower(rsVirturalPath[nExt+1]) != L'j' ||
			tolower(rsVirturalPath[nExt+2]) != L'p' ||
			tolower(rsVirturalPath[nExt+3]) != L'e' ||
			tolower(rsVirturalPath[nExt+4]) == L'g')
			return false;
	}
	else return false;

	rThumbnailFile.reset(new EncapsulatedFile(rsVirturalPath, IPtrPlisgoFSFile(m_ThumbnailPlaceHolder)));

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
