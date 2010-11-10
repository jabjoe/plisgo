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
#include "shlobj.h"
#include "PlisgoFSFolderReg.h"
#include "PlisgoExtractIcon.h"
#include "PlisgoExtractImage.h"

// CPlisgoFolder

#undef Shell_GetCachedImageIndex 

const CLSID	CLSID_PlisgoFolder		= {0xADA19F85,0xEEB6,0x46F2,{0xB8,0xB2,0x2B,0xD9,0x77,0x93,0x4A,0x79}};

const IID	IID_IShellFolderView	= {0x37A378C0,0xF82D,0x11CE,{0xAE,0x65,0x08,0x00,0x2B,0x2E,0x12,0x62}};




CPlisgoFolder::CPlisgoFolder()
{
	m_pIDL = NULL;

	m_pCurrent = NULL;

	m_pDefaultSFVCB = NULL;
}

CPlisgoFolder::~CPlisgoFolder()
{
	if (m_pCurrent != NULL)
		m_pCurrent->Release();

	if (m_pIDL != NULL)
		ILFree(m_pIDL);

	m_pCurrent = NULL;
	m_pIDL = NULL;

	if (m_pDefaultSFVCB != NULL)
		m_pDefaultSFVCB->Release();
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



static HRESULT CreatePlisgoExtractIcon_(void** ppResult, IPtrPlisgoFSRoot plisgoFSFolder, const std::wstring& rsPath)
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


static HRESULT CreatePlisgoExtractImage_(void** ppResult, IPtrPlisgoFSRoot plisgoFSFolder, const std::wstring& rsPath)
{
	CComObject<CPlisgoExtractImage>* pPlisgoExtractImage;

	HRESULT hr = CComObject<CPlisgoExtractImage>::CreateInstance ( &pPlisgoExtractImage );

	if ( !FAILED(hr) && pPlisgoExtractImage != NULL)
	{
		pPlisgoExtractImage->AddRef();

		pPlisgoExtractImage->Init(rsPath, plisgoFSFolder);

		hr = pPlisgoExtractImage->QueryInterface(IID_IExtractImage, ppResult);

		pPlisgoExtractImage->Release();
	}
	
	return hr;
}


HRESULT		 CPlisgoFolder::CreatePlisgoExtractImage(LPCITEMIDLIST pIDL, void** ppResult)
{
	WCHAR sName[MAX_PATH];

	HRESULT hr = GetItemName(pIDL, sName, MAX_PATH);

	if (FAILED(hr))
		return hr;

	return CreatePlisgoExtractImage_( ppResult, m_PlisgoFSFolder, m_sPath + L"\\" += sName);
}


HRESULT		 CPlisgoFolder::CreatePlisgoExtractIcon(LPCITEMIDLIST pIDL, void** ppResult)
{
	WCHAR sName[MAX_PATH];

	HRESULT hr = GetItemName(pIDL, sName, MAX_PATH);

	if (FAILED(hr))
		return hr;

	return CreatePlisgoExtractIcon_(ppResult, m_PlisgoFSFolder, m_sPath + L"\\" += sName);
}


static const GUID IID_HACK_IShellView3 = {0xEC39FA88,0xF8AF,0x41CF,{0x84,0x21,0x38,0xBE,0xD2,0x8F,0x46,0x73}};



	// IShellFolderViewCB
HRESULT	CPlisgoFolder::MessageSFVCB(UINT	uMsg,
									WPARAM	wParam,
									LPARAM	lParam)
{
	if (uMsg == SFVM_GETNOTIFY)
	{
		*reinterpret_cast<PIDLIST_ABSOLUTE*>(wParam) = GetIDList();
        *reinterpret_cast<LONG*>(lParam) = SHCNE_ATTRIBUTES | SHCNE_CREATE | SHCNE_DELETE | SHCNE_RENAMEITEM |
											SHCNE_UPDATEITEM | SHCNE_UPDATEDIR | SHCNE_MKDIR | SHCNE_RMDIR;

		return S_OK;
	}
	else if (uMsg == SFVM_QUERYFSNOTIFY)
	{
		SHChangeNotifyEntry* pChange = (SHChangeNotifyEntry*)lParam;

		pChange->fRecursive = TRUE;
		pChange->pidl = GetIDList();

		return S_OK;
	}
	else if (uMsg == SFVM_FSNOTIFY)
	{
		return S_OK;
	}
	else if (uMsg == SFVM_BACKGROUNDENUM)
	{
		return S_OK;
	}
	else if (uMsg == SFVM_DEFVIEWMODE)
	{
		*((FOLDERVIEWMODE*)lParam) = FVM_DETAILS;

		return S_OK;
	}

	/*if (m_pDefaultSFVCB != NULL)
		return m_pDefaultSFVCB->MessageSFVCB(uMsg, wParam, lParam);
*/
	return E_NOTIMPL;

}


STDMETHODIMP CPlisgoFolder::CreateViewObject(HWND hWnd, REFIID rIID, void** ppResult)
{
	if (ppResult == NULL)
		return E_INVALIDARG;

	if (rIID == IID_IShellView	||
		rIID == IID_IShellView2 ||
		rIID == IID_HACK_IShellView3)
	{
		/*SFV_CREATE csfv = {0};
		
		csfv.cbSize = sizeof(csfv);

		QueryInterface(IID_IShellFolder, (void**)&csfv.pshf);

		HRESULT hr =  SHCreateShellFolderView(&csfv, (LPSHELLVIEW*)ppResult);

		if (FAILED(hr))
			return hr;

		IShellFolderView *pSFV;

		if (SUCCEEDED(((IShellView*)*ppResult)->QueryInterface( IID_IShellFolderView, (LPVOID*)&pSFV)))
		{
			if (m_pDefaultSFVCB == NULL)
			{
				pSFV->SetCallback(this, &m_pDefaultSFVCB);
			}

			pSFV->Release();
		}

		if (csfv.pshf != NULL)
			csfv.pshf->Release();

		return S_OK;*/
/*
		HRESULT hr = m_pCurrent->CreateViewObject(hWnd, IID_IShellView, ppResult);

		if (FAILED(hr))
			return hr;

		IShellFolderView *pSFV;

		if (SUCCEEDED(((IShellView*)*ppResult)->QueryInterface( IID_IShellFolderView, (LPVOID*)&pSFV)))
		{
			if (m_pDefaultSFVCB == NULL)
			{
				pSFV->SetCallback(this, &m_pDefaultSFVCB);
			}

			pSFV->Release();
		}

		return S_OK;*/

		SFV_CREATE csfv = {0};
		
		csfv.cbSize = sizeof(csfv);

		QueryInterface(IID_IShellFolder, (void**)&csfv.pshf);
		QueryInterface(IID_IShellFolderViewCB, (void**)&csfv.psfvcb);

		HRESULT hr =  SHCreateShellFolderView(&csfv, (LPSHELLVIEW*)ppResult);

		if (csfv.pshf != NULL)
			csfv.pshf->Release();

		if (csfv.psfvcb != NULL)
			csfv.psfvcb->Release();

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
				return CreatePlisgoExtractIcon_(ppResult, plisgoFSFolder, m_sPath);
		}
	}

	if (rIID == IID_IExtractImage)
		return E_NOTIMPL;

	if (rIID == IID_IShellIcon)
		return QueryInterface(IID_IShellIcon, ppResult);

	return m_pCurrent->CreateViewObject(hWnd, rIID, ppResult);
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
		rIID == IID_IPersistFolder ||
		IsSupportEncapsulatedInterface(rIID))
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
		return CreatePlisgoExtractIcon(*pIDL, ppResult);

	if (rIID == IID_IExtractImage && cidl == 1)
		return CreatePlisgoExtractImage(*pIDL, ppResult);

	return m_pCurrent->GetUIObjectOf(hWnd, cidl, pIDL, rIID, pnReserved, ppResult);
}


STDMETHODIMP CPlisgoFolder::GetThumbnailHandler(PCUITEMID_CHILD pIDL,
								IBindCtx *pbc,
								REFIID rIID,
								void **ppResult)
{
	if (rIID == IID_IExtractImage)
		return CreatePlisgoExtractImage(pIDL, ppResult);

	if (rIID == IID_IThumbnailProvider)
	{
		HRESULT hResult = CreatePlisgoExtractImage(pIDL, ppResult);
		
		if (hResult != S_OK)
			return hResult;

		IUnknown* pUnknown = (IUnknown*)*ppResult;

		hResult = pUnknown->QueryInterface(IID_IThumbnailProvider, ppResult);

		pUnknown->Release();

		return hResult;
	}

	if (rIID == IID_IExtractIconW)
		return CreatePlisgoExtractIcon(pIDL, ppResult);

	PrintInterfaceName(rIID);

	return E_NOTIMPL;
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

	if (nAttr&FILE_ATTRIBUTE_DIRECTORY)
		*rgfInOut |= SFGAO_FILESYSANCESTOR|SFGAO_FOLDER|SFGAO_HASSUBFOLDER|SFGAO_BROWSABLE; //Too much work to check every folder for sub folders

	if (nAttr&FILE_ATTRIBUTE_HIDDEN)
		*rgfInOut |= SFGAO_HIDDEN|SFGAO_GHOSTED;

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

	IconLocation Location;

	if (!m_PlisgoFSFolder->GetPathIconLocation(Location, m_sPath + L"\\" += sName,  bOpen))
		return E_FAIL;

	int nSysIndex = Shell_GetCachedImageIndex(Location.sPath.c_str(), Location.nIndex, 0);

	if (nSysIndex == -1)
		return E_FAIL;

	*lpIconIndex = nSysIndex;

	return S_OK;
}




HRESULT			CPlisgoFolder::GetDefaultColumn(DWORD dwReserved, ULONG *pSort, ULONG *pDisplay)
{
	if (pSort == NULL || pDisplay == NULL)
		return E_POINTER;

	//name column
	*pSort		= 0;
	*pDisplay	= 0;

	return S_OK;

}

#define MAX_STD_COLUMN 8


HRESULT			CPlisgoFolder::GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags)
{
	if (m_PlisgoFSFolder.get() == NULL)
		return m_pCurrent->GetDefaultColumnState(iColumn, pcsFlags);

	if (pcsFlags == NULL)
		return E_POINTER;

	if (iColumn < MAX_STD_COLUMN)
	{
		HRESULT hResult = m_pCurrent->GetDefaultColumnState(iColumn, pcsFlags);

		if (FAILED(hResult))
			return hResult;

		if (m_PlisgoFSFolder->IsStandardColumnDisabled(iColumn))
			*pcsFlags |= SHCOLSTATE_SECONDARYUI;
		else
			*pcsFlags &= ~(SHCOLSTATE_SECONDARYUI|SHCOLSTATE_HIDDEN);

		return S_OK;
	}

	int nPlisgoColumnIndex = (int)iColumn-MAX_STD_COLUMN;

	if (nPlisgoColumnIndex >= (int)m_PlisgoFSFolder->GetColumnNum())
		return E_FAIL;

	*pcsFlags = SHCOLSTATE_TYPE_STR|SHCOLSTATE_ONBYDEFAULT|SHCOLSTATE_SLOW;

	return S_OK;
}


HRESULT			CPlisgoFolder::InitPlisgoColumnShellDetails(const std::wstring& rsEntry, int nPlisgoColumnIndex, SHELLDETAILS *psd)
{
	size_t nSize = sizeof(WCHAR)*(rsEntry.length()+1);

	psd->str.uType = STRRET_WSTR;

	HRESULT hResult = SHStrDup(rsEntry.c_str(), &psd->str.pOleStr);

	if (hResult != S_OK)
		return hResult;

	psd->cxChar = (int)rsEntry.length()+1;

	PlisgoFSRoot::ColumnAlignment alig = m_PlisgoFSFolder->GetColumnAlignment(nPlisgoColumnIndex);

	if (alig == PlisgoFSRoot::LEFT)
		psd->fmt = LVCFMT_LEFT;
	else if (alig == PlisgoFSRoot::CENTER)
		psd->fmt = LVCFMT_CENTER;
	else
		psd->fmt = LVCFMT_RIGHT;	

	return hResult;
}


HRESULT			CPlisgoFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
	if (m_PlisgoFSFolder.get() == NULL || iColumn < MAX_STD_COLUMN)
		return m_pCurrent->GetDetailsOf(pidl, iColumn, psd);

	if (psd == NULL)
		return E_POINTER;

	int nPlisgoColumnIndex = (int)iColumn-MAX_STD_COLUMN;

	if (nPlisgoColumnIndex >= (int)m_PlisgoFSFolder->GetColumnNum())
		return E_FAIL;
	

	if (pidl != NULL)
	{
		std::wstring sPath;

		HRESULT hResult = GetPathOf(sPath, pidl);

		if (FAILED(hResult))
			return hResult;

		std::wstring sResult;

		m_PlisgoFSFolder->GetPathColumnTextEntry(sResult, sPath, nPlisgoColumnIndex);

		return InitPlisgoColumnShellDetails(sResult, nPlisgoColumnIndex, psd);
	}
	else
	{
		int nWidth = m_PlisgoFSFolder->GetColumnDefaultWidth(nPlisgoColumnIndex);

		if (nWidth != -1)
			psd->cxChar = nWidth;

		return InitPlisgoColumnShellDetails(	m_PlisgoFSFolder->GetColumnHeader(nPlisgoColumnIndex),
												nPlisgoColumnIndex, psd);
	}
}
