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
#include "PlisgoFolder.h"
#include "PlisgoView.h"
#include "shlobj.h"
#include "PlisgoFSFolderReg.h"
#include "PlisgoExtractIcon.h"

// CPlisgoFolder

#undef Shell_GetCachedImageIndex 


CPlisgoFolder::CPlisgoFolder()
{
	m_pIDL = NULL;

	m_pCurrent = NULL;
}

CPlisgoFolder::~CPlisgoFolder()
{
	if (m_pCurrent != NULL)
		m_pCurrent->Release();

	if (m_pIDL != NULL)
		ILFree(m_pIDL);

	m_pCurrent = NULL;
	m_pIDL = NULL;
}


STDMETHODIMP CPlisgoFolder::GetClassID ( CLSID* pClsid )
{
    if ( NULL == pClsid )
        return E_POINTER;

    *pClsid = CLSID_PlisgoFolder;
    return S_OK;
}


// IPersistFolder
STDMETHODIMP CPlisgoFolder::Initialize(LPCITEMIDLIST pIDL)
{
	std::wstring sPath;

	HRESULT nResult = GetWStringPathFromIDL(sPath, pIDL);

	if (nResult != S_OK)
		return nResult;

	return Initialize(pIDL, sPath);
}


HRESULT				CPlisgoFolder::Initialize(LPCITEMIDLIST pIDL, const std::wstring& rsPath)
{
	m_pIDL = ILCloneFull(pIDL);

	if (m_pCurrent != NULL)
		m_pCurrent->Release();

	HRESULT nResult = GetShellIShellFolder2Implimentation(&m_pCurrent);

	if ( !FAILED(nResult) )
	{
		CComQIPtr<IPersistFolder> pPersistFolder(m_pCurrent);

		if ( pPersistFolder.p != NULL)
		{
			nResult = pPersistFolder->Initialize(pIDL);

			m_sPath = rsPath;

			if ( !FAILED(nResult) )
				m_PlisgoFSFolder = PlisgoFSFolderReg::GetSingleton()->GetPlisgoFSRoot(rsPath.c_str());
		}
	}

	return nResult;
}



static HRESULT CreatePlisgoExtractIcon_(HWND hWnd, void** ppResult, IPtrPlisgoFSRoot plisgoFSFolder, const std::wstring& rsPath)
{
	CComPtr<CPlisgoExtractIcon> pPlisgoExtractIcon;

	HRESULT hr = CoCreateInstance(CLSID_PlisgoExtractIcon, NULL, CLSCTX_ALL, IID_IPlisgoExtractIcon, (void**)&pPlisgoExtractIcon);

	if ( !FAILED(hr) && pPlisgoExtractIcon != NULL)
	{
		pPlisgoExtractIcon->Init(rsPath, plisgoFSFolder);

		hr = pPlisgoExtractIcon->QueryInterface(IID_IExtractIcon, ppResult);
	}
	
	return hr;
}



HRESULT		 CPlisgoFolder::CreatePlisgoExtractIcon(LPCITEMIDLIST pIDL, HWND hWnd, void** ppResult)
{
	WCHAR sName[MAX_PATH];

	HRESULT hr = GetItemName(pIDL, sName, MAX_PATH);

	if (FAILED(hr))
		return hr;

	return CreatePlisgoExtractIcon_(hWnd, ppResult, m_PlisgoFSFolder, m_sPath + L"\\" += sName);
}

static const GUID IID_HACK_IShellView3 = {0xEC39FA88,0xF8AF,0x41CF,{0x84,0x21,0x38,0xBE,0xD2,0x8F,0x46,0x73}};



STDMETHODIMP CPlisgoFolder::CreateViewObject(HWND hWnd, REFIID rIID, void** ppResult)
{
	if (rIID == IID_IShellView	||
		rIID == IID_IShellView2 ||
		rIID == IID_IDropTarget ||
		rIID == IID_IOleWindow	||
		rIID == IID_IOleCommandTarget)
	{
		HRESULT hr;
		CComObject<CPlisgoView>* pPlisgoView;

		hr = CComObject<CPlisgoView>::CreateInstance ( &pPlisgoView );

		if ( FAILED(hr) || pPlisgoView == NULL)
			return hr;

		pPlisgoView->AddRef();

		if (pPlisgoView->Init(this))
			hr = pPlisgoView->QueryInterface(rIID, ppResult);
		else
			hr = m_pCurrent->CreateViewObject(hWnd, rIID, ppResult);

		pPlisgoView->Release();

		return hr;
	}

	if (rIID == IID_IExtractIconA)
		return E_NOINTERFACE;

	if (rIID == IID_IExtractIconW && m_PlisgoFSFolder.get() != NULL)
	{
		const size_t nSlash = m_sPath.rfind(L'\\');

		if (nSlash != -1)
		{
			const std::wstring sPath(m_sPath.begin(),m_sPath.begin()+nSlash);

			IPtrPlisgoFSRoot plisgoFSFolder = PlisgoFSFolderReg::GetSingleton()->GetPlisgoFSRoot(sPath.c_str());

			if (plisgoFSFolder.get() != NULL)
				return CreatePlisgoExtractIcon_(hWnd, ppResult, plisgoFSFolder, m_sPath);
		}
	}

	if (rIID == IID_HACK_IShellView3)
		return E_NOINTERFACE;

	return m_pCurrent->CreateViewObject(hWnd, rIID, ppResult);
}


STDMETHODIMP CPlisgoFolder::EnumObjects(HWND hWnd, DWORD nFlags, LPENUMIDLIST* ppResult)
{
	HRESULT hr = m_pCurrent->EnumObjects(hWnd, nFlags, ppResult);

	return hr;
}


HRESULT		 CPlisgoFolder::CreateIPlisgoFolder(LPCITEMIDLIST pIDL, LPBC pBC, REFIID rIID, void** ppResult)
{
	if (rIID == IID_IPlisgoFolder ||
		rIID == IID_IUnknown ||
		rIID == IID_IPersist ||
		rIID == IID_IShellFolder ||
		rIID == IID_IShellFolder2 ||
		rIID == IID_IPersistFolder2 ||
		rIID == IID_IPersistFolder ||
		rIID == IID_IShellIcon)
	{
		HRESULT hr;

		DWORD nAttr = 0;

		hr = GetAttributesOf(1, &pIDL, &nAttr);

		if ( FAILED(hr))
			return hr;

		if (!(nAttr&SFGAO_FOLDER))
			return E_NOTIMPL; //Not for me

		WCHAR sName[MAX_PATH];

		hr = GetItemName(pIDL, sName, MAX_PATH);

		if ( FAILED(hr))
			return hr;

		CComObject<CPlisgoFolder>* pShellFolder;

		hr = CComObject<CPlisgoFolder>::CreateInstance ( &pShellFolder );

		if ( FAILED(hr) || pShellFolder == NULL)
			return hr;

		pShellFolder->AddRef();

		LPITEMIDLIST pChildFull = ILCombine(m_pIDL, pIDL);

		if (pChildFull != NULL)
		{
			hr = pShellFolder->Initialize(pChildFull, m_sPath + L"\\" += sName);

			ILFree(pChildFull);
		}
		else hr = E_FAIL;

		if ( !FAILED(hr) )
			hr = pShellFolder->QueryInterface(rIID, ppResult);
		
		pShellFolder->Release();
		
		
		return hr;
	}

	return E_NOTIMPL;
}


STDMETHODIMP CPlisgoFolder::BindToObject(LPCITEMIDLIST pIDL, LPBC pBC, REFIID rIID, void** ppResult)
{
	HRESULT hr = CreateIPlisgoFolder(pIDL, pBC, rIID, ppResult);

	if (hr != E_NOTIMPL)
		return hr;

	return m_pCurrent->BindToObject(pIDL, pBC, rIID, ppResult);
}


STDMETHODIMP CPlisgoFolder::BindToStorage(LPCITEMIDLIST pIDL, LPBC pBC, REFIID rIID, void** ppResult)
{
	HRESULT hr = CreateIPlisgoFolder(pIDL, pBC, rIID, ppResult);

	if (hr != E_NOTIMPL)
		return hr;

	return m_pCurrent->BindToStorage(pIDL, pBC, rIID, ppResult);
}


STDMETHODIMP CPlisgoFolder::GetUIObjectOf(HWND hWnd, UINT cidl, LPCITEMIDLIST* pIDL, REFIID rIID, LPUINT pnReserved, void** ppResult)
{
	if (rIID == IID_IExtractIconA)
		return E_NOINTERFACE;

	if (m_PlisgoFSFolder.get() != NULL && rIID == IID_IExtractIconW && cidl == 1)
		return CreatePlisgoExtractIcon(*pIDL, hWnd, ppResult);

	return m_pCurrent->GetUIObjectOf(hWnd, cidl, pIDL, rIID, pnReserved, ppResult);
}


HRESULT		 CPlisgoFolder::GetTextOfColumn(PCUITEMID_CHILD pIDL, UINT nColumn, WCHAR* sBuffer, size_t nBufferSize) const
{
	SHELLDETAILS sd;

	LRESULT hr = const_cast<CPlisgoFolder*>(this)->GetDetailsOf(pIDL, nColumn, &sd);

	if (SUCCEEDED(hr) && hr != S_FALSE)
	{
		hr = StrRetToBuf(&sd.str, pIDL, sBuffer, nBufferSize);

		if (sd.str.uType == STRRET_WSTR)
			CoTaskMemFree(sd.str.pOleStr);
	}

	return hr;
}


HRESULT			CPlisgoFolder::GetItemName(PCUITEMID_CHILD pIDL, WCHAR* sBuffer, size_t nBufferSize) const
{
	assert(pIDL != NULL && sBuffer != NULL);

	STRRET name;

	HRESULT hr = const_cast<CPlisgoFolder*>(this)->GetDisplayNameOf(pIDL, SHGDN_INFOLDER | SHGDN_FORPARSING, &name);

	if (FAILED(hr))
		return hr;

	hr = StrRetToBuf(&name, pIDL, sBuffer, nBufferSize);

	if (name.uType == STRRET_WSTR)
		CoTaskMemFree(name.pOleStr);

	if (FAILED(hr))
		return hr;

	return S_OK;
}


// IShellIcon
HRESULT			CPlisgoFolder::GetIconOf( LPCITEMIDLIST pIDL, UINT nFlags, LPINT lpIconIndex)
{
	if (lpIconIndex == NULL)
		return ERROR_INVALID_PARAMETER;

	if (m_PlisgoFSFolder.get() == NULL)
	{
		IShellIcon* pInterface = NULL;

		HRESULT hr = m_pCurrent->QueryInterface(IID_IShellIcon, (void**)&pInterface);

		if ( FAILED(hr) )
			return hr;

		hr = pInterface->GetIconOf(pIDL, nFlags, lpIconIndex);

		pInterface->Release();

		return hr;
	}

	WCHAR sName[MAX_PATH];

	HRESULT hr = GetItemName(pIDL, sName, MAX_PATH);

	if (hr != S_OK)
		return hr;

	const bool bOpen = ((nFlags&GIL_OPENICON) == GIL_OPENICON);

	IPtrRefIconList iconList = m_PlisgoFSFolder->GetFSIconRegistry()->GetMainIconRegistry()->GetRefIconList(16);

	if (iconList.get() == NULL)
		return E_NOTIMPL;

	IconLocation Location;

	if (!m_PlisgoFSFolder->GetPathIconLocation(Location, m_sPath + L"\\" += sName, iconList, bOpen))
		return E_FAIL;

	int nSysIndex = Shell_GetCachedImageIndex(Location.sPath.c_str(), Location.nIndex, 0);

	if (nSysIndex == -1)
		return E_FAIL;

	*lpIconIndex = nSysIndex;

	return S_OK;
}