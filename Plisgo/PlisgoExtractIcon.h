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

#include "PlisgoFSFolder.h"
#include "IconRegistry.h"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

class IconRegistry;

extern const CLSID	CLSID_PlisgoExtractIcon;

class ATL_NO_VTABLE CPlisgoExtractIcon :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPlisgoExtractIcon, &CLSID_PlisgoExtractIcon>,
    public IPersistFile,
    public IExtractIcon
{
public:

	void Init(const std::wstring& sPath, IPtrPlisgoFSRoot localFSFolder);

BEGIN_COM_MAP(CPlisgoExtractIcon)
    COM_INTERFACE_ENTRY(IPersist)
    COM_INTERFACE_ENTRY(IPersistFile)
    COM_INTERFACE_ENTRY(IExtractIcon)
END_COM_MAP()

	DECLARE_REGISTRY_RESOURCEID(IDR_PLISGOEXTRACTICON)

public:

    // IPersist
	STDMETHOD(GetClassID)( CLSID* pClsid)				{ return E_NOTIMPL; }

    // IPersistFile
    STDMETHOD(IsDirty)()								{ return S_OK; }
    STDMETHOD(Save)( LPCOLESTR, BOOL )					{ return E_NOTIMPL; }
    STDMETHOD(SaveCompleted)( LPCOLESTR )				{ return E_NOTIMPL; }
    STDMETHOD(GetCurFile)( LPOLESTR* )					{ return E_NOTIMPL; }

    STDMETHOD(Load)( LPCOLESTR wszFile, DWORD nMode)	{ return E_NOTIMPL; }

    // IExtractIcon
    STDMETHOD(GetIconLocation)( UINT uFlags, LPTSTR szIconFile, UINT cchMax, int* piIndex, UINT* pwFlags );

    STDMETHOD(Extract)( LPCTSTR pszFile, UINT nIconIndex, HICON* pHiconLarge,
						HICON* pHiconSmall, UINT nIconSize );

protected:

	std::wstring			m_sPath;
	IPtrPlisgoFSRoot		m_PlisgoFSFolder;
	bool					m_bOpen;
	bool					m_bExtract;
};
