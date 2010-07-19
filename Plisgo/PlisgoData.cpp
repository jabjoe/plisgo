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

#include "stdafx.h"
#include "PlisgoData.h"
#include "PlisgoEnumFORMATETC.h"
#include "PlisgoView.h"


const CLSID	CLSID_PlisgoData		= {0x3F57F545,0x286B,0x4881,{0x84,0x84,0x46,0x68,0x12,0x61,0x0A,0xC8}};


CPlisgoData::~CPlisgoData()
{
	if (m_pPlisgoView != NULL)
		m_pPlisgoView->Release();
}


void CPlisgoData::Init(CPlisgoView* pPlisgoView)
{
	assert(pPlisgoView != NULL);

	m_pPlisgoView = pPlisgoView;

	m_pPlisgoView->AddRef();
}


STDMETHODIMP CPlisgoData::GetData( FORMATETC* pFormatetcIn, STGMEDIUM* pMedium)
{
	if (pFormatetcIn == NULL || pMedium == NULL)
		return E_INVALIDARG;

	if( pFormatetcIn->cfFormat != CF_HDROP )
		return DV_E_FORMATETC;

	if (m_pPlisgoView == NULL)
		return E_FAIL;

	HGLOBAL hDropData = m_pPlisgoView->GetSelectionAsDropData();

	if (hDropData == NULL)
		return E_FAIL;

    ::ZeroMemory(pMedium, sizeof(STGMEDIUM));

	pMedium->tymed = TYMED_HGLOBAL;
	pMedium->hGlobal = hDropData;
	pMedium->pUnkForRelease = NULL;

	return S_OK;
}


STDMETHODIMP CPlisgoData::QueryGetData( FORMATETC* pFormatetc)
{
	if(pFormatetc == NULL)
		return E_INVALIDARG;
	
    if( pFormatetc->cfFormat != CF_HDROP )
         return DV_E_FORMATETC;

	if(pFormatetc->ptd!=NULL)
		return E_INVALIDARG;
	
    if( (pFormatetc->dwAspect & DVASPECT_CONTENT)==0 ) 
        return DV_E_DVASPECT;
	
	if(pFormatetc->lindex !=-1)
		return DV_E_LINDEX;

	if((pFormatetc->tymed & TYMED_HGLOBAL) ==0)
		return DV_E_TYMED;

	return S_OK;
}


STDMETHODIMP CPlisgoData::EnumFormatEtc( DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc)
{
    CComObject<CPlisgoEnumFORMATETC>* pPlisgoEnumFormatEtc;

    HRESULT hr = CComObject<CPlisgoEnumFORMATETC>::CreateInstance(&pPlisgoEnumFormatEtc);

	if ( FAILED(hr) )
		return hr;

	pPlisgoEnumFormatEtc->AddRef();

    hr = pPlisgoEnumFormatEtc->QueryInterface(IID_IEnumFORMATETC, (LPVOID *)ppEnumFormatEtc);

	pPlisgoEnumFormatEtc->Release();

	return hr;
}