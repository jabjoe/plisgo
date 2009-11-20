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
#pragma once

#include <tlhelp32.h>


class ProcessesFolder : public PlisgoFSFolder, public IShellInfoFetcher
{
public:
	ProcessesFolder();

	void						RefreshProcesses();

	// PlisgoFSFolder interface

	virtual LPCWSTR				GetName() const			{ return L"processes"; }
	virtual const std::wstring&	GetPath() const			{ return m_sPath; }
	virtual DWORD				GetAttributes() const	{ return FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_READONLY; }

	virtual IPtrPlisgoFSFile	GetDescendant(LPCWSTR sPath) const;
	virtual bool				ForEachChild(EachChild& rEachChild) const;
	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sName) const;
	virtual UINT				GetChildNum() const;


	//IShellInfoFetcher Interface

	virtual bool				ReadShelled(const std::wstring& rsFolderPath, IShellInfoFetcher::BasicFolder* pResult) const;

	virtual bool				GetColumnEntry(const std::wstring& rsFilePath, const int nColumnIndex, std::wstring& rsResult) const;
	virtual bool				GetOverlayIcon(const std::wstring& rsFilePath, IconLocation& rResult) const;
	virtual bool				GetCustomIcon(const std::wstring& rsFilePath, IconLocation& rResult) const;
	virtual bool				GetThumbnail(const std::wstring& rsFilePath, const std::wstring& rsVirturalPath, IPtrPlisgoFSFile& rThumbnailFile) const;

private:

	bool						ResolveDevicePath(std::wstring& rsPath) const;
	void						LoadDriveDevices();

	void					Update();
	IPtrPlisgoFSFile		GetProcessFile(const DWORD nProcessID) const;

	mutable boost::shared_mutex		m_Mutex;
	std::map<DWORD, PROCESSENTRY32>	m_Processes;
	std::wstring					m_sPath;
	PlisgoFSFileList				m_Extras;

	typedef std::pair<std::wstring,std::wstring>	DriveEntry;

	std::vector<DriveEntry>				m_DriveDevices;

	boost::shared_ptr<PlisgoFSDataFile>	m_ThumbnailPlaceHolder;
};

