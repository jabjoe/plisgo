/*
	Copyright 2009 Eurocom Entertainment Software Limited

    This file is part of PlisgoFSLib.

    Eurocom Entertainment Software Limited, hereby disclaims all
    copyright interest in “PlisgoFSLib” written by Joe Burmeister.

    PlisgoFSLib is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

    PlisgoFSLib is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
	License along with PlisgoFSLib.  If not, see
	<http://www.gnu.org/licenses/>.
*/

#pragma once

#include "PlisgoVFS.h"



class IconLocation
{
public:

	IconLocation()		{ m_nList = m_nIndex = -1; }

	void				Set(const std::wstring& rsFile, const int nIndex = 0)	{ m_sFile = rsFile; m_nIndex = nIndex; }
	void				Set(const int nList, const int nIndex)					{ m_sFile.clear(); m_nList = nList; m_nIndex = nIndex; }

	bool				GetText(std::string& rsResult) const;

	bool				IsFromFile() const		{ return (m_sFile.length() != 0); }
	const std::wstring& GetFile() const			{ return m_sFile; }

	int					GetListIndex() const	{ return m_nList; }
	int					GetIndex() const		{ return m_nIndex; }

private:
	std::wstring	m_sFile;
	int				m_nList;
	int				m_nIndex;
};

class RootPlisgoFSFolder;

typedef boost::shared_ptr<RootPlisgoFSFolder>		IPtrRootPlisgoFSFolder;


class IShellInfoFetcher : public boost::enable_shared_from_this<IShellInfoFetcher>
{
public:
	virtual ~IShellInfoFetcher() {}

	virtual LPCWSTR	GetFFSName() const = 0;

	virtual bool IsShelled(IPtrPlisgoFSFile& rFile) const = 0; 

	virtual bool GetColumnEntry(IPtrPlisgoFSFile& rFile, const int nColumnIndex, std::wstring& rsResult) const = 0;
	virtual bool GetOverlayIcon(IPtrPlisgoFSFile& rFile, IconLocation& rResult) const = 0;
	virtual bool GetCustomIcon(IPtrPlisgoFSFile& rFile, IconLocation& rResult) const = 0;
	virtual bool GetThumbnail(IPtrPlisgoFSFile& rFile, std::wstring& rsExt, IPtrPlisgoFSFile& rThumbnailFile) const = 0;

	//Create a plisgo folder set up with this IShellInfoFetcher
	virtual IPtrRootPlisgoFSFolder	CreatePlisgoFolder(const std::wstring& rsPath, IPtrPlisgoVFS& rVFS);
};


typedef boost::shared_ptr<IShellInfoFetcher>		IPtrIShellInfoFetcher;


template<class T>
class StringEventRedirect : public StringEvent
{
public:
	typedef bool (T::*EventMethod)(LPCWSTR sPath);

	StringEventRedirect(EventMethod Method, T* pObj)
	{
		m_Method	= Method;
		m_pObj		= pObj;
	}

	virtual bool Do(LPCWSTR sPath)
	{
		return (m_pObj->*m_Method)(sPath);
	}

protected:
	EventMethod	m_Method;
	T*			m_pObj;
};





class RootPlisgoFSFolder : public PlisgoFSStorageFolder
{
	friend class ShellInfoFolder;

public:

	RootPlisgoFSFolder(const std::wstring& rsPath, LPCWSTR sFSName, IPtrPlisgoVFS& rVFS, IPtrIShellInfoFetcher ShellInfoFetcher = IPtrIShellInfoFetcher());

	bool						AddPngIcons(HINSTANCE hExeHandle, int nListIndex, LPCWSTR sName)							{ return AddIcons(hExeHandle, nListIndex, sName, L"PNG", L"png"); }
	bool						AddIcons(int nListIndex, const std::wstring& sFilename);

	void						AddSeparator(int nParentMenu = -1)	{ AddMenu(NULL,IPtrStringEvent(),nParentMenu); }

	int							AddMenu(LPCWSTR			sText,
										IPtrStringEvent	onClickEvent = IPtrStringEvent(),
										int				nParentMenu = -1,
										IPtrStringEvent enabledEvent = IPtrStringEvent(),
										int				nIconList = -1,
										int				nIconIndex = -1);

	void						AddCustomFolderIcon(int	nClosedIconList, int nClosedIconIndex,
													int	nOpenIconList, int nOpenIconIndex);

	void						AddCustomDefaultIcon(int nList, int nIndex);

	void						AddCustomExtensionIcon(LPCWSTR sExt, int nList, int nIndex);

	bool						AddColumn(std::wstring sHeader);

	enum ColumnAlignment
	{
		LEFT	= -1,
		CENTER	= 0,
		RIGHT	= 1
	};

	bool						SetColumnAlignment(UINT nColumnIndex, ColumnAlignment eAlignment);

	enum ColumnType
	{
		TEXT	= 0, //default
		INT		= 1,
		FLOAT	= 2
	};

	bool						SetColumnType(UINT nColumnIndex, ColumnType eType);

	bool						SetColumnDefaultWidth(UINT nColumnIndex, int nWidth);

	enum StdColumn
	{
		StdColumn_Size		= 1,
		StdColumn_Type		= 2,
		StdColumn_Date_Mod	= 3,
		StdColumn_Date_Crt	= 4,
		StdColumn_Date_Acc	= 5,
		StdColumn_Attrib	= 6
	};

	void						DisableStandardColumn(StdColumn eStdColumn);

	bool						EnableThumbnails();
	bool						EnableCustomIcons();
	bool						EnableOverlays();
	
	UINT						GetColumnNum() const;
	UINT						GetIconListNum() const;
	bool						HasThumbnails() const;
	bool						HasCustomIcons() const;
	bool						HasOverlays() const;
	bool						HasCustomDefaultIcon() const;
	bool						HasCustomFolderIcons() const;


	IShellInfoFetcher*			GetShellInfoFetcher() const { return m_IShellInfoFetcher.get(); }
	IPtrPlisgoVFS				GetVFS() const				{ return m_VFS; }

protected:

	IPtrPlisgoFSFile			FindMenu(int nMenu);

private:
	void						EnableCustomShell();
	IPtrPlisgoFSFile			GetCustomTypeIconsFolder();

	bool						AddIcons(HINSTANCE hExeHandle, int nListIndex, LPCWSTR sName, LPCWSTR sType, LPCWSTR sExt);

	mutable boost::shared_mutex	m_Mutex;

	ULONG						m_nRootMenuNum;
	UINT						m_nIconListsNum;

	IPtrIShellInfoFetcher		m_IShellInfoFetcher;

	bool						m_bHasCustomDefaultIcon;
	bool						m_bHasCustomFolderIcons;

	bool						m_bEnableThumbnails;
	bool						m_bEnableCustomIcons;
	bool						m_bEnableOverlays;
	UINT						m_nColumnNum;

	bool						m_DisabledStandardColumn[8];

	IPtrPlisgoVFS				m_VFS;
	std::wstring				m_sPath;
};
