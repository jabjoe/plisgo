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


class ProcessesFolderShellInterface : public IShellInfoFetcher
{
public:
	ProcessesFolderShellInterface();

	virtual LPCWSTR	GetFFSName() const		{ return L"ProcessFS"; }

	virtual bool IsShelled(IPtrPlisgoFSFile& rFile) const	{ return true; }

	virtual bool GetColumnEntry(IPtrPlisgoFSFile& rFile, const int nColumnIndex, std::wstring& rsResult) const;
	virtual bool GetOverlayIcon(IPtrPlisgoFSFile& rFile, IconLocation& rResult) const;
	virtual bool GetCustomIcon(IPtrPlisgoFSFile& rFile, IconLocation& rResult) const;
	virtual bool GetThumbnail(IPtrPlisgoFSFile& rFile, std::wstring& rsExt, IPtrPlisgoFSFile& rThumbnailFile) const;

	virtual IPtrRootPlisgoFSFolder	CreatePlisgoFolder(IPtrPlisgoVFS& rVFS, const std::wstring& rsPath, bool bDoMounts);

private:

	boost::shared_ptr<PlisgoFSDataFile>	m_ThumbnailPlaceHolder;
};


class ProcessesFolder : public PlisgoFSFolder
{
public:

	ProcessesFolder();
	~ProcessesFolder();

	virtual DWORD				GetAttributes() const	{ return FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_READONLY; }

	virtual int					GetChildren(ChildNames& rChildren) const;
	virtual IPtrPlisgoFSFile	GetChild(LPCWSTR sName) const;

private:
	std::wstring					m_sPath;
	PlisgoFSFileList				m_Extras;
};

