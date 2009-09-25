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

#pragma once
#include "resource.h"       // main symbols

#include "Plisgo_i.h"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif


#include "PlisgoFSMenu.h"



class ATL_NO_VTABLE CPlisgoMenu :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPlisgoMenu, &CLSID_PlisgoMenu>,
	public IDispatchImpl<IPlisgoMenu, &IID_IPlisgoMenu, &LIBID_PlisgoLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IShellExtInit,
	public IContextMenu3
{
public:
	CPlisgoMenu()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PLISGOMENU)


BEGIN_COM_MAP(CPlisgoMenu)
	COM_INTERFACE_ENTRY(IPlisgoMenu)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IShellExtInit)
	COM_INTERFACE_ENTRY(IContextMenu)
	COM_INTERFACE_ENTRY(IContextMenu2)
	COM_INTERFACE_ENTRY(IContextMenu3)
END_COM_MAP()


	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

public:

    // IShellExtInit
	STDMETHOD(Initialize)(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);


    // IContextMenu
	STDMETHOD(GetCommandString)(UINT_PTR idCmd,
								UINT uFlags,
								UINT *pwReserved,
								LPSTR pszName,
								UINT cchMax)	{ return E_NOTIMPL; }

    // IContextMenu
	STDMETHOD(InvokeCommand)( LPCMINVOKECOMMANDINFO pICI);

    // IContextMenu
	STDMETHOD(QueryContextMenu)(HMENU hMenu,
								UINT indexMenu,
								UINT idCmdFirst,
								UINT idCmdLast,
								UINT uFlags);

    // IContextMenu2
	STDMETHOD(HandleMenuMsg)( UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		LRESULT nUnused;

		return HandleMenuMsg2(uMsg, wParam, lParam, &nUnused);
	}

    // IContextMenu3
	STDMETHOD(HandleMenuMsg2)( UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);

protected:

	bool	InsertPlisgoMenuToMenu(HMENU hMenu, const IPtrPlisgoFSMenu& rPlisgoMenu, UINT& rnID, int nPos);

private:

	UINT								m_nBeginID;
	UINT								m_nEndID;
	std::map<UINT, IPtrPlisgoFSMenu>	m_nIDToPlisgoMenu;
	IPtrPlisgoFSMenuList				m_PlisgoFSMenuItems;
	IPtrPlisgoFSRoot					m_RootFSFolder;
	WStringList							m_Selection;
};

OBJECT_ENTRY_AUTO(__uuidof(PlisgoMenu), CPlisgoMenu)
