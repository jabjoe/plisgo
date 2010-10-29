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
#include "PlisgoExtractImage.h"
#include "PlisgoFSFolderReg.h"


const CLSID CLSID_PlisgoExtractImage =		{0xdb3a3ef7,0xffdd,0x48ab,{0xb8,0x35,0xb9,0xe7,0x16,0x5a,0xf3,0x69}};


CPlisgoExtractImage::CPlisgoExtractImage()
{
	m_nColorDepth = 0;
	m_Size.cx = m_Size.cy = 0;
}


CPlisgoExtractImage::~CPlisgoExtractImage()
{
}



void CPlisgoExtractImage::Init(const std::wstring& sPath, IPtrPlisgoFSRoot localFSFolder)
{
	assert(sPath.length() > 0 && localFSFolder.get() != NULL);

	m_PlisgoFSFolder = localFSFolder;

	m_sPath = sPath;

	m_nColorDepth = 0;
	m_Size.cx = m_Size.cy = 0;
}


	// IInitializeWithFile
STDMETHODIMP CPlisgoExtractImage::Initialize(LPCWSTR sFilePath, DWORD )
{
	m_sPath = sFilePath;

	return S_OK;
}

    // IExtractImage
STDMETHODIMP CPlisgoExtractImage::GetLocation(	LPWSTR		pszPathBuffer,
												DWORD		cchMax,
												DWORD*		pdwPriority,
												const SIZE*	prgSize,
												DWORD		dwRecClrDepth,
												DWORD*		pdwFlags)
{

	if (pszPathBuffer == NULL)
		return E_POINTER;

	if (prgSize != NULL)
		m_Size = *prgSize;
	else
		ZeroMemory(&m_Size, sizeof(m_Size));

	if (!InitResult(m_Size.cy))
		return E_NOTIMPL;

	wcscpy_s(pszPathBuffer, cchMax, m_sResult.c_str());

	BOOL bIsAsync = FALSE;

	if (pdwFlags != NULL)
	{
		bIsAsync = *pdwFlags & IEIFLAG_ASYNC;

		*pdwFlags = IEIFLAG_ASPECT|IEIFLAG_OFFLINE;
	}

	if (pdwPriority != NULL)
		*pdwPriority = IEIT_PRIORITY_NORMAL;


	m_nColorDepth = (WORD)dwRecClrDepth;

	return (bIsAsync)?E_PENDING:S_OK;
}


STDMETHODIMP CPlisgoExtractImage::Extract( HBITMAP *phBmpImage )
{
	if (phBmpImage == NULL)
		return E_POINTER;

	*phBmpImage = ExtractBitmap(m_sResult, m_Size.cx, m_Size.cy, m_nColorDepth);

	if (*phBmpImage == NULL)
		return E_NOTIMPL;
	else
		return S_OK;
}


STDMETHODIMP CPlisgoExtractImage::GetThumbnail(UINT cx, HBITMAP *phBmpImage, WTS_ALPHATYPE *pdwAlpha)
{
	if (phBmpImage == NULL)
		return E_POINTER;

	if (!InitResult(cx))
		return E_NOTIMPL;

	if (pdwAlpha != NULL)
		*pdwAlpha = WTSAT_ARGB;

	*phBmpImage = ExtractBitmap(m_sResult, m_Size.cx, m_Size.cy, 32);

	if (*phBmpImage == NULL)
		return E_NOTIMPL;

	return S_OK;
}


bool	CPlisgoExtractImage::InitResult(UINT hHeight)
{
	m_Size.cx = m_Size.cy = hHeight;

	if (!m_PlisgoFSFolder->GetThumbnailFile(m_sPath, m_sResult))
		return false;

	IconLocation overlay;

	if (m_PlisgoFSFolder->GetFileOverlay(m_sPath, overlay))
	{
		IconLocation iconVersion;

		IconRegistry* pIconRegistry = FSIconRegistriesManager::GetSingleton()->GetIconRegistry();

		if (pIconRegistry->GetAsWindowsIconLocation(iconVersion, m_sResult, hHeight))
		{
			IconLocation result;

			if (pIconRegistry->MakeOverlaid(result, iconVersion, overlay))
			{
				m_sResult = result.sPath;
				
				if (hHeight > 256) //Not possible
					m_Size.cx = m_Size.cy = 256;
			}
		}
	}

	return true;
}
