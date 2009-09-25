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




class ATL_NO_VTABLE CPlisgoEnumFORMATETC :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPlisgoEnumFORMATETC, &CLSID_PlisgoEnumFORMATETC>,
	public IDispatchImpl<IPlisgoEnumFORMATETC, &IID_IPlisgoEnumFORMATETC, &LIBID_PlisgoLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IEnumFORMATETC
{
public:
	CPlisgoEnumFORMATETC()
	{
		m_nEnumPos = 0;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PLISGOENUMFORMATETC)


BEGIN_COM_MAP(CPlisgoEnumFORMATETC)
	COM_INTERFACE_ENTRY(IPlisgoEnumFORMATETC)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IEnumFORMATETC)
END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

	//IEnumFORMATETC
	
	STDMETHOD(Next)(ULONG, LPFORMATETC pFormatetc, ULONG* pnCopied)
	{
		m_nEnumPos++;

		if (m_nEnumPos != 1)
		{
		  if( pnCopied!=NULL ) 
			  *pnCopied = 0;

		  return S_FALSE;
		}

		pFormatetc->cfFormat = CF_HDROP;
		pFormatetc->ptd = NULL;
		pFormatetc->dwAspect = DVASPECT_CONTENT;
		pFormatetc->lindex = -1;
		pFormatetc->tymed = TYMED_HGLOBAL;

		if( pnCopied!=NULL ) 
			*pnCopied = 1;

		return S_OK;
	}

	STDMETHOD(Skip)(ULONG n)
	{
		m_nEnumPos += n;
		return S_OK;
	}

	STDMETHOD(Reset)(void)
	{
		m_nEnumPos = 0;
		return S_OK;
	}

	STDMETHOD(Clone)(LPENUMFORMATETC*)														{ return E_NOTIMPL; }
private:

	UINT m_nEnumPos;
};

OBJECT_ENTRY_AUTO(__uuidof(PlisgoEnumFORMATETC), CPlisgoEnumFORMATETC)
