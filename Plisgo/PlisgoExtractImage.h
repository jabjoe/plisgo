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
#include <Thumbcache.h>


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

class IconRegistry;

extern const CLSID	CLSID_PlisgoExtractImage;

class ATL_NO_VTABLE CPlisgoExtractImage :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPlisgoExtractImage, &CLSID_PlisgoExtractImage>,
    public IExtractImage,
	public IThumbnailProvider,
	public IInitializeWithFile
{
public:

	CPlisgoExtractImage();
	~CPlisgoExtractImage();

	void Init(const std::wstring& sPath, IPtrPlisgoFSRoot localFSFolder);

BEGIN_COM_MAP(CPlisgoExtractImage)
    COM_INTERFACE_ENTRY(IExtractImage)
    COM_INTERFACE_ENTRY(IThumbnailProvider)
	COM_INTERFACE_ENTRY(IInitializeWithFile)
END_COM_MAP()

	DECLARE_REGISTRY_RESOURCEID(IDR_PLISGOEXTRACTIMAGE)

public:
    // IExtractImage

    STDMETHOD(GetLocation)(	PWSTR pszPathBuffer,
							DWORD		cchMax,
							DWORD*		pdwPriority,
							const SIZE*	prgSize,
							DWORD		dwRecClrDepth,
							DWORD*		pdwFlags);

    STDMETHOD(Extract)( HBITMAP *phBmpImage );

	// IInitializeWithFile
	STDMETHOD(Initialize)(LPCWSTR sFilePath, DWORD grfMode);

	// IThumbnailProvider
	STDMETHOD(GetThumbnail)(UINT cx, HBITMAP *phBmpImage, WTS_ALPHATYPE *pdwAlpha);

protected:

	bool	InitResult(UINT hHeight);

	std::wstring			m_sPath;
	std::wstring			m_sResult;
	IPtrPlisgoFSRoot		m_PlisgoFSFolder;
	SIZE					m_Size;
	WORD					m_nColorDepth;
};
