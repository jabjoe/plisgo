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

const CLSID	CLSID_PlisgoFolder		= {0xADA19F85,0xEEB6,0x46F2,{0xB8,0xB2,0x2B,0xD9,0x77,0x93,0x4A,0x79}};



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

	IPtrPlisgoFSRoot plisgoFSFolder = PlisgoFSFolderReg::GetSingleton()->GetPlisgoFSRoot(sPath.c_str());

	if (plisgoFSFolder.get() == NULL)
		return E_FAIL;

	return Initialize(pIDL, sPath, plisgoFSFolder);
}


HRESULT				CPlisgoFolder::Initialize(LPCITEMIDLIST pIDL, const std::wstring& rsPath, IPtrPlisgoFSRoot& rPlisgoFSFolder )
{
	if (m_pCurrent != NULL)
		return CO_E_ALREADYINITIALIZED;

	HRESULT hr = GetShellIShellFolder2Implimentation(&m_pCurrent);

	if ( SUCCEEDED(hr) )
	{
		CComQIPtr<IPersistFolder> pPersistFolder(m_pCurrent);

		if ( pPersistFolder.p != NULL)
		{
			hr = pPersistFolder->Initialize(pIDL);

			if (SUCCEEDED(hr))
			{
				m_sPath				= rsPath;
				m_PlisgoFSFolder	= rPlisgoFSFolder;
				m_pIDL				= ILCloneFull(pIDL);
			}
		}
		else hr = E_FAIL;
	}

	return hr;
}



static HRESULT CreatePlisgoExtractIcon_(HWND hWnd, void** ppResult, IPtrPlisgoFSRoot plisgoFSFolder, const std::wstring& rsPath)
{
	CComObject<CPlisgoExtractIcon>* pPlisgoExtractIcon;

	HRESULT hr = CComObject<CPlisgoExtractIcon>::CreateInstance ( &pPlisgoExtractIcon );

	if ( !FAILED(hr) && pPlisgoExtractIcon != NULL)
	{
		pPlisgoExtractIcon->AddRef();

		pPlisgoExtractIcon->Init(rsPath, plisgoFSFolder);

		hr = pPlisgoExtractIcon->QueryInterface(IID_IExtractIcon, ppResult);

		pPlisgoExtractIcon->Release();
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
	if (ppResult == NULL)
		return E_INVALIDARG;

	if (rIID == IID_HACK_IShellView3)
		return E_NOINTERFACE;


	*ppResult = NULL;

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

	if (rIID == IID_IContextMenu ||
		rIID == IID_IContextMenu2 ||
		rIID == IID_IContextMenu3)
	{
		return m_pCurrent->CreateViewObject(hWnd, rIID, ppResult);
	}

	if (rIID == IID_IShellIcon)
		return QueryInterface(IID_IShellIcon, ppResult);


	HRESULT hr = m_pCurrent->CreateViewObject(hWnd, rIID, ppResult);

	return hr;
}


STDMETHODIMP CPlisgoFolder::EnumObjects(HWND hWnd, DWORD nFlags, LPENUMIDLIST* ppResult)
{
	HRESULT hr = m_pCurrent->EnumObjects(hWnd, nFlags, ppResult);

	return hr;
}


HRESULT		 CPlisgoFolder::CreateIPlisgoFolder(LPCITEMIDLIST pIDL, LPBC pBC, REFIID rIID, void** ppResult)
{
	//Only if a folder is being asked for
	if (rIID == IID_IShellFolder ||
		rIID == IID_IShellFolder2 ||
		rIID == IID_IPersistFolder2 ||
		rIID == IID_IPersistFolder)
	{
		std::wstring sPath;

		HRESULT hr = GetPathOf(sPath, pIDL);

		if (FAILED(hr))
			return hr;

		IPtrPlisgoFSRoot plisgoFSFolder = PlisgoFSFolderReg::GetSingleton()->GetPlisgoFSRoot(sPath.c_str());

		if (plisgoFSFolder.get() != NULL)
		{
			const DWORD nAttr = GetFileAttributes(sPath.c_str());

			if (nAttr == INVALID_FILE_ATTRIBUTES || !(nAttr&FILE_ATTRIBUTE_DIRECTORY))
				return E_FAIL;

			CComObject<CPlisgoFolder>* pShellFolder;

			hr = CComObject<CPlisgoFolder>::CreateInstance ( &pShellFolder );

			if ( FAILED(hr))
				return hr;

			assert(pShellFolder != NULL);

			pShellFolder->AddRef();

			LPITEMIDLIST pChildFull = ILCombine(m_pIDL, pIDL);

			if (pChildFull != NULL)
			{
				hr = pShellFolder->Initialize(pChildFull, sPath, plisgoFSFolder);

				ILFree(pChildFull);
			}
			else hr = E_FAIL;

			if ( SUCCEEDED(hr) )
				hr = pShellFolder->QueryInterface(rIID, ppResult);
			
			pShellFolder->Release();
					
			return hr;
		}
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
	SHELLDETAILS sd = {0};

	HRESULT hr = const_cast<CPlisgoFolder*>(this)->GetDetailsOf(pIDL, nColumn, &sd);

	if (SUCCEEDED(hr) && hr != S_FALSE)
	{
		hr = StrRetToBuf(&sd.str, pIDL, sBuffer, (UINT)nBufferSize);

		if (sd.str.uType == STRRET_WSTR)
			CoTaskMemFree(sd.str.pOleStr);
	}

	return hr;
}


HRESULT			CPlisgoFolder::GetItemName(PCUITEMID_CHILD pIDL, WCHAR* sBuffer, size_t nBufferSize) const
{
	if (pIDL == NULL && sBuffer == NULL)
		return E_INVALIDARG;

	STRRET name;

	HRESULT hr = const_cast<CPlisgoFolder*>(this)->GetDisplayNameOf(pIDL, SHGDN_INFOLDER | SHGDN_FORPARSING, &name);

	if (FAILED(hr))
		return hr;

	hr = StrRetToBuf(&name, pIDL, sBuffer, (UINT)nBufferSize);

	if (name.uType == STRRET_WSTR)
		CoTaskMemFree(name.pOleStr);

	if (FAILED(hr))
		return hr;

	return S_OK;
}


HRESULT			CPlisgoFolder::GetPathOf(std::wstring& rsResult, LPCITEMIDLIST pIDL)
{
	if (pIDL == NULL)
		return E_INVALIDARG;

	STRRET name;

	HRESULT hr = const_cast<CPlisgoFolder*>(this)->GetDisplayNameOf(pIDL, SHGDN_FORPARSING, &name);

	if (FAILED(hr))
		return hr;

	if (name.uType == STRRET_WSTR)
	{
		rsResult = name.pOleStr;
		CoTaskMemFree(name.pOleStr);
	}
	else if (name.uType == STRRET_CSTR)
	{
		ToWide(rsResult, name.cStr);
	}
	else if (name.uType == STRRET_OFFSET)
	{
		ToWide(rsResult, (LPCSTR)((LPBYTE)pIDL) + name.uOffset);
	}

	return S_OK;
}


HRESULT			CPlisgoFolder::GetAttributesOf(LPCITEMIDLIST pIDL, LPDWORD rgfInOut)
{
	if (rgfInOut == NULL)
		return E_INVALIDARG;

	std::wstring sPath;

	HRESULT hr = GetPathOf(sPath, pIDL);

	if (FAILED(hr))
		return hr;

	const DWORD nAttr = GetFileAttributes(sPath.c_str());

	if (nAttr == INVALID_FILE_ATTRIBUTES)
		return m_pCurrent->GetAttributesOf(1, &pIDL, rgfInOut); //Do orignal to extract attribute from LPCITEMIDLIST directly

	*rgfInOut = SFGAO_STORAGE | SFGAO_STORAGEANCESTOR | SFGAO_FILESYSTEM | SFGAO_HASPROPSHEET | SFGAO_DROPTARGET |
		SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_CANLINK | SFGAO_CANCOPY | SFGAO_CANMOVE;

	if (nAttr&FILE_ATTRIBUTE_READONLY)
		*rgfInOut |= SFGAO_READONLY;

	//if (nAttr&FILE_ATTRIBUTE_OFFLINE) //It's probably virtual if used with plisgo, which means slow
		*rgfInOut |= SFGAO_ISSLOW;

	if (nAttr&FILE_ATTRIBUTE_DIRECTORY)
		*rgfInOut |= SFGAO_FILESYSANCESTOR|SFGAO_FOLDER|SFGAO_HASSUBFOLDER|SFGAO_BROWSABLE; //Too much work to check every folder for sub folders

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