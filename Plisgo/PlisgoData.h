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


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

extern const CLSID	CLSID_PlisgoData;

class CPlisgoView;

// CPlisgoData

class ATL_NO_VTABLE CPlisgoData :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPlisgoData, &CLSID_PlisgoData>,
	public IDataObject
{
public:
	CPlisgoData()
	{
	}

	~CPlisgoData();

DECLARE_REGISTRY_RESOURCEID(IDR_PLISGODATA)


BEGIN_COM_MAP(CPlisgoData)
	COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()


	void Init(CPlisgoView* pPlisgoView);

	//IDataObject

    STDMETHOD(GetData)( FORMATETC* pFormatetcIn, STGMEDIUM* pMedium);
    STDMETHOD(QueryGetData)( FORMATETC* pFormatetc);
    STDMETHOD(EnumFormatEtc)( DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc);
	STDMETHOD(GetDataHere)( FORMATETC*, STGMEDIUM* )										{ return E_NOTIMPL; }
    STDMETHOD(GetCanonicalFormatEtc)( FORMATETC*, FORMATETC*)								{ return E_NOTIMPL; }
    STDMETHOD(SetData)( FORMATETC*, STGMEDIUM*, BOOL )										{ return E_NOTIMPL; }
    STDMETHOD(DAdvise)( FORMATETC*, DWORD , IAdviseSink*, DWORD*)							{ return E_NOTIMPL; }
    STDMETHOD(DUnadvise)( DWORD )															{ return E_NOTIMPL; }
    STDMETHOD(EnumDAdvise)( IEnumSTATDATA **)												{ return E_NOTIMPL; }

private:
	
    CPlisgoView*					m_pPlisgoView;
};

