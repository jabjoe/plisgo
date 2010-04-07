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
#include "PlisgoFSFolder.h"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif




class ATL_NO_VTABLE CPlisgoFolder :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPlisgoFolder, &CLSID_PlisgoFolder>,
	public IDispatchImpl<IPlisgoFolder, &IID_IPlisgoFolder, &LIBID_PlisgoLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IShellFolder2,
	public IPersistFolder2,
	public IShellIcon
{
public:
	CPlisgoFolder();
	~CPlisgoFolder();

DECLARE_REGISTRY_RESOURCEID(IDR_PLISGOFOLDER)


BEGIN_COM_MAP(CPlisgoFolder)
	COM_INTERFACE_ENTRY(IPlisgoFolder)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IShellFolder)
    COM_INTERFACE_ENTRY(IShellFolder2)
    COM_INTERFACE_ENTRY(IPersistFolder2)
    COM_INTERFACE_ENTRY(IPersistFolder)
    COM_INTERFACE_ENTRY(IPersist)
    COM_INTERFACE_ENTRY(IShellIcon)
END_COM_MAP()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

	static HRESULT InternalQueryInterface(void* pThis,
										  const _ATL_INTMAP_ENTRY* pEntries,
										  REFIID iid,
										  void** ppvObject )
	{
		if (iid == IID_IPlisgoFolder || iid == IID_IDispatch || 
			iid == IID_IShellFolder || iid == IID_IShellFolder2 ||
			iid == IID_IPersistFolder3 || iid == IID_IPersistFolder2 ||
			iid == IID_IPersistFolder || iid == IID_IPersist ||
			iid == IID_IShellIcon || iid == IID_IUnknown)
			return CComObjectRootBase::InternalQueryInterface(pThis, pEntries, iid, ppvObject);

		HRESULT hr = E_NOTIMPL;

		IShellFolder2* pCaptured = ((CPlisgoFolder*)pThis)->GetCaptured();

		if (pCaptured != NULL)
			hr = pCaptured->QueryInterface(iid, ppvObject);
		
		return  hr;
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

    STDMETHOD(CompareIDs) (LPARAM lParam, LPCITEMIDLIST pIDL1, LPCITEMIDLIST pIDL2)
	{
		return m_pCurrent->CompareIDs(lParam, pIDL1, pIDL2);
	}

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

	STDMETHOD(GetDefaultColumn) (DWORD dwReserved, ULONG *pSort, ULONG *pDisplay)
	{
		return m_pCurrent->GetDefaultColumn(dwReserved, pSort, pDisplay);
	}

	STDMETHOD(GetDefaultColumnState) (UINT iColumn, SHCOLSTATEF *pcsFlags)
	{
		return m_pCurrent->GetDefaultColumnState(iColumn,pcsFlags);
	}
	
	STDMETHOD(GetDefaultSearchGUID) (GUID *pguid)
	{
		return m_pCurrent->GetDefaultSearchGUID(pguid);
	}

	STDMETHOD(GetDetailsEx) (PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
	{
		return m_pCurrent->GetDetailsEx(pidl, pscid, pv);
	}

	STDMETHOD(GetDetailsOf) (PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
	{
		return m_pCurrent->GetDetailsOf(pidl, iColumn, psd);
	}


	STDMETHOD(MapColumnToSCID) (UINT iColumn, SHCOLUMNID *pscid)
	{
		return m_pCurrent->MapColumnToSCID(iColumn, pscid);
	}

	// IShellIcon
	STDMETHOD(GetIconOf)( LPCITEMIDLIST pIDL, UINT nFlags, LPINT lpIconIndex);

	HRESULT	GetPathOf(std::wstring& rsResult, LPCITEMIDLIST pIDL)
	{
		if (pIDL == NULL)
			return E_INVALIDARG;

		WCHAR sName[MAX_PATH];

		HRESULT nResult = GetItemName(pIDL, sName, MAX_PATH);

		if (FAILED(nResult))
			return nResult;

		rsResult = m_sPath;
		rsResult += L"\\";
		rsResult += sName;

		return S_OK;
	}

	
    HRESULT GetAttributesOf(LPCITEMIDLIST pIDL, LPDWORD rgfInOut)
	{
		if (rgfInOut == NULL)
			return E_INVALIDARG;

		std::wstring sPath;

		HRESULT hr = GetPathOf(sPath, pIDL);

		if (FAILED(hr))
			return hr;

		const DWORD nAttr = GetFileAttributes(sPath.c_str());

		if (nAttr == INVALID_FILE_ATTRIBUTES)
			return STG_E_FILENOTFOUND;

		*rgfInOut = SFGAO_ISSLOW | SFGAO_STORAGE | SFGAO_FILESYSTEM;

		if (nAttr&FILE_ATTRIBUTE_DIRECTORY)
			*rgfInOut |= SFGAO_FOLDER|SFGAO_HASSUBFOLDER; //Too much work to check every folder for sub folders

		return S_OK;
	}



	IPtrPlisgoFSRoot	GetPlisgoFSLocal() const	{ return m_PlisgoFSFolder; }

	HRESULT				GetTextOfColumn(PCUITEMID_CHILD pIDL, UINT nColumn, WCHAR* sBuffer, size_t nBufferSize) const;
	HRESULT				GetItemName(PCUITEMID_CHILD pIDL, WCHAR* sBuffer, size_t nBufferSize) const;

	IShellFolder2*		GetCaptured() const		{ return m_pCurrent; }
	LPITEMIDLIST		GetIDList() const		{ return m_pIDL; }

	const std::wstring&	GetPath() const			{ return m_sPath; }

protected:
	HRESULT				Initialize(LPCITEMIDLIST pIDL, const std::wstring& rsPath, IPtrPlisgoFSRoot& rPlisgoFSFolder );

private:

	HRESULT				CreateIPlisgoFolder(LPCITEMIDLIST pIDL, LPBC pBC, REFIID rIID, void** ppResult);
	HRESULT				CreatePlisgoExtractIcon(LPCITEMIDLIST pIDL, HWND hWnd, void** ppResult);

	std::wstring			m_sPath;
	IPtrPlisgoFSRoot		m_PlisgoFSFolder;
	IShellFolder2*			m_pCurrent;
	LPITEMIDLIST			m_pIDL;
};

OBJECT_ENTRY_AUTO(__uuidof(PlisgoFolder), CPlisgoFolder)
