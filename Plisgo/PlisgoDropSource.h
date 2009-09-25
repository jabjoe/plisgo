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




class ATL_NO_VTABLE CPlisgoDropSource :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPlisgoDropSource, &CLSID_PlisgoDropSource>,
	public IDispatchImpl<IPlisgoDropSource, &IID_IPlisgoDropSource, &LIBID_PlisgoLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IDropSource
{
public:
	CPlisgoDropSource()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PLISGODROPSOURCE)


BEGIN_COM_MAP(CPlisgoDropSource)
	COM_INTERFACE_ENTRY(IPlisgoDropSource)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IDropSource)
END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

	//IDropSource
    STDMETHOD(QueryContinueDrag)(BOOL bEsc, DWORD dwKeyState)
    {
        if( bEsc ) 
        	return ResultFromScode(DRAGDROP_S_CANCEL);

        if( (dwKeyState & MK_LBUTTON)==0 ) 
        	return ResultFromScode(DRAGDROP_S_DROP);

        return S_OK;
    }

    STDMETHOD(GiveFeedback)(DWORD)		{ return ResultFromScode(DRAGDROP_S_USEDEFAULTCURSORS); }
};

OBJECT_ENTRY_AUTO(__uuidof(PlisgoDropSource), CPlisgoDropSource)
