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


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

extern const CLSID	CLSID_PlisgoFolder;


class ATL_NO_VTABLE CPlisgoFolder :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPlisgoFolder, &CLSID_PlisgoFolder>,
	public IShellFolder2,
	public IPersistFolder2,
	public IShellIcon,
	public IShellFolderViewCB,
	public IThumbnailHandlerFactory
{
public:
	CPlisgoFolder();
	~CPlisgoFolder();


BEGIN_COM_MAP(CPlisgoFolder)
    COM_INTERFACE_ENTRY(IShellFolder)
    COM_INTERFACE_ENTRY(IShellFolder2)
    COM_INTERFACE_ENTRY(IPersistFolder2)
    COM_INTERFACE_ENTRY(IPersistFolder)
    COM_INTERFACE_ENTRY(IPersist)
    COM_INTERFACE_ENTRY(IShellIcon)
    COM_INTERFACE_ENTRY(IShellFolderViewCB)
    COM_INTERFACE_ENTRY(IThumbnailHandlerFactory)
END_COM_MAP()

	DECLARE_REGISTRY_RESOURCEID(IDR_PLISGOFOLDER)


	static bool		IsSupportEncapsulatedInterface(REFIID rIID)
	{
		return (rIID == IID_IPropertyStore ||
				rIID == IID_IPropertyStoreFactory ||
				rIID == IID_IObjectProvider ||
				rIID == IID_IExplorerPaneVisibility ||
				rIID == IID_IParentAndItem ||
				rIID == IID_IObjectWithSite || 
				rIID == IID_IPersistIDList 
				);
	}


	static HRESULT InternalQueryInterface(void* pThis,
										  const _ATL_INTMAP_ENTRY* pEntries,
										  REFIID rIID,
										  void** ppvObject )
	{
		HRESULT hr = CComObjectRootBase::InternalQueryInterface(pThis, pEntries, rIID, ppvObject);

		if (SUCCEEDED(hr))
			return hr;

		if (!IsSupportEncapsulatedInterface(rIID))
			return E_NOTIMPL;
		
		hr = E_NOTIMPL;

		IShellFolder2* pCaptured = ((CPlisgoFolder*)pThis)->GetCaptured();

		if (pCaptured != NULL)
			hr = pCaptured->QueryInterface(rIID, ppvObject);
		
		return hr;
	}

public:
    // IPersist
    STDMETHOD(GetClassID)(CLSID*);

    // IPersistFolder
    STDMETHOD(Initialize)(LPCITEMIDLIST pIDL);
	
    // IPersistFolder2
	STDMETHOD(GetCurFolder)(LPITEMIDLIST* ppIDL)
	{
		*ppIDL = ILCloneFull(m_pIDL);

		return S_OK;
	}



    // IShellFolder
    STDMETHOD(BindToObject) (LPCITEMIDLIST pIDL, LPBC pBC, REFIID rIID, void** ppResult);

	STDMETHOD(BindToStorage) (LPCITEMIDLIST pIDL, LPBC pBC, REFIID rIID, void** ppResult);

    STDMETHOD(CompareIDs) (LPARAM lParam, LPCITEMIDLIST pIDL1, LPCITEMIDLIST pIDL2);

    STDMETHOD(CreateViewObject) (HWND hWnd, REFIID rIID, void** ppResult);

    STDMETHOD(EnumObjects) (HWND hWnd, DWORD nFlags, LPENUMIDLIST* ppResult);
	
    STDMETHOD(GetAttributesOf) (UINT cidl, LPCITEMIDLIST* apidl, LPDWORD rgfInOut)
	{
		if (rgfInOut == NULL)
			return E_INVALIDARG;

		for(UINT n = 0; n < cidl; ++n)
		{
			HRESULT hr = GetAttributesOf(apidl[n], rgfInOut);

			if (FAILED(hr))
				return hr;
		}

		return S_OK;
	}


    STDMETHOD(GetDisplayNameOf) (LPCITEMIDLIST pIDL, DWORD nFlahs, LPSTRRET name)
	{
		HRESULT hr = m_pCurrent->GetDisplayNameOf(pIDL, nFlahs, name);

		return hr;
	}

    STDMETHOD(GetUIObjectOf) (HWND hWnd, UINT cidl, LPCITEMIDLIST* apidl, REFIID rIID, LPUINT pnReserved, void** ppResult);

    STDMETHOD(ParseDisplayName) (HWND hWnd, LPBC pBC, LPOLESTR pszDisplayName, LPDWORD pnEaten, LPITEMIDLIST* pIDL, LPDWORD pnAttr)
	{
		return m_pCurrent->ParseDisplayName(hWnd, pBC, pszDisplayName, pnEaten, pIDL, pnAttr);
	}

    STDMETHOD(SetNameOf) (HWND hWnd, LPCITEMIDLIST pIDL, LPCOLESTR name, DWORD nFlags, LPITEMIDLIST* ppidlOut)
	{
		return m_pCurrent->SetNameOf(hWnd, pIDL, name, nFlags, ppidlOut);
	}



    // IShellFolder2

	STDMETHOD(EnumSearches) (IEnumExtraSearch **ppEnum)
	{
		return m_pCurrent->EnumSearches(ppEnum);
	}

	STDMETHOD(GetDefaultSearchGUID) (GUID *pguid)
	{
		return m_pCurrent->GetDefaultSearchGUID(pguid);
	}


	STDMETHOD(GetDefaultColumn) (DWORD dwReserved, ULONG *pSort, ULONG *pDisplay);
	STDMETHOD(GetDefaultColumnState) (UINT iColumn, SHCOLSTATEF *pcsFlags);
	STDMETHOD(GetDetailsOf) (PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd);
	STDMETHOD(MapColumnToSCID)(UINT , SHCOLUMNID*)	{ return E_NOTIMPL; }

	STDMETHOD(GetDetailsEx) (PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
	{
		return m_pCurrent->GetDetailsEx(pidl, pscid, pv);
	}
	// IShellIcon
	STDMETHOD(GetIconOf)( LPCITEMIDLIST pIDL, UINT nFlags, LPINT lpIconIndex);


	// IShellFolderViewCB
	STDMETHOD(MessageSFVCB)(UINT	uMsg,
							WPARAM	wParam,
							LPARAM	lParam);

	// IThumbnailHandlerFactory 
	STDMETHOD(GetThumbnailHandler)(	PCUITEMID_CHILD pIDL,
									IBindCtx *pbc,
									REFIID rIID,
									void **ppResult);

	HRESULT	GetPathOf(std::wstring& rsResult, LPCITEMIDLIST pIDL);
	
    HRESULT GetAttributesOf(LPCITEMIDLIST pIDL, LPDWORD rgfInOut);



	IPtrPlisgoFSRoot	GetPlisgoFSLocal() const	{ return m_PlisgoFSFolder; }

	//HRESULT				GetTextOfColumn(PCUITEMID_CHILD pIDL, UINT nColumn, WCHAR* sBuffer, size_t nBufferSize) const;
	HRESULT				GetItemName(PCUITEMID_CHILD pIDL, WCHAR* sBuffer, size_t nBufferSize) const;

	IShellFolder2*		GetCaptured() const		{ return m_pCurrent; }
	LPITEMIDLIST		GetIDList() const		{ return m_pIDL; }

	const std::wstring&	GetPath() const			{ return m_sPath; }

protected:
	HRESULT				Initialize(LPCITEMIDLIST pIDL, const std::wstring& rsPath, IPtrPlisgoFSRoot& rPlisgoFSFolder );

private:
	HRESULT				InitPlisgoColumnShellDetails(const std::wstring& rsEntry, int nPlisgoColumnIndex, SHELLDETAILS *psd);

	HRESULT				CreateIPlisgoFolder(LPCITEMIDLIST pIDL, LPBC pBC, REFIID rIID, void** ppResult);
	HRESULT				CreatePlisgoExtractIcon(LPCITEMIDLIST pIDL, void** ppResult);
	HRESULT				CreatePlisgoExtractImage(LPCITEMIDLIST pIDL, void** ppResult);

	std::wstring			m_sPath;
	IPtrPlisgoFSRoot		m_PlisgoFSFolder;
	IShellFolder2*			m_pCurrent;
	LPITEMIDLIST			m_pIDL;
};
