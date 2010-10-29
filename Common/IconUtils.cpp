/*
	Copyright 2009 Eurocom Entertainment Software Limited

    This file is part of Plisgo's Utils.

    Eurocom Entertainment Software Limited, hereby disclaims all
    copyright interest in “Plisgo's Utils” written by Joe Burmeister.

    PlisgoFSLib is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

    Plisgo's Utils is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
	License along with Plisgo's Utils.  If not, see
	<http://www.gnu.org/licenses/>.
*/

#include "Utils.h"
#include "PathUtils.h"
#include "IconUtils.h"


static HBITMAP		CreateMeABitmap(HDC hDC, LONG nWidth, LONG nHeight, WORD nDepth, DWORD** ppBits )
{
	BITMAPINFO bi = {0};

	bi.bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth		= nWidth;
	bi.bmiHeader.biHeight		= nHeight;
	bi.bmiHeader.biPlanes		= 1;
	bi.bmiHeader.biBitCount		= nDepth;
	bi.bmiHeader.biCompression	= BI_RGB;
	bi.bmiHeader.biSizeImage	= nWidth * nHeight * 4;

	return CreateDIBSection(hDC, (BITMAPINFO *)&bi, DIB_RGB_COLORS, (void **)ppBits, NULL, 0);
}


HBITMAP			CreateAlphaBitmap(HDC hDC, LONG nWidth, LONG nHeight, DWORD** ppBits )
{
	return CreateMeABitmap(hDC, nWidth, nHeight, 32, ppBits);
}


HBITMAP	CreateBitmapOfHeight(HBITMAP hBitmap, const UINT nAimHeight, const WORD nColorDepth)
{
	BITMAP bm = {0};

	if (GetObject(hBitmap, sizeof(BITMAP), &bm) != sizeof(BITMAP))
		return NULL;

	UINT nHeight = bm.bmHeight;
	UINT nWidth = bm.bmWidth;

	if (nHeight == nAimHeight && nWidth == nAimHeight && bm.bmBitsPixel == nColorDepth)
		return hBitmap;


	UINT nTargetWidth;
	UINT nTargetHeight;

	if (nHeight==nWidth)
	{
		nTargetHeight = nAimHeight;
		nTargetWidth = nAimHeight;
	}
	else if (nHeight>nWidth)
	{
		nTargetWidth = (UINT)(nWidth * (nAimHeight/(float)nHeight));
		nTargetHeight = nAimHeight;
	}
	else
	{
		nTargetHeight = (UINT)(nHeight * (nAimHeight/(float)nWidth));
		nTargetWidth = nAimHeight;
	}

	HBITMAP hNewBitmap = NULL;
	HDC hSrcDC = NULL;
	HDC hDstDC = NULL;
	HGDIOBJ hOldSrcSel = NULL;
	HGDIOBJ hOldDstSel = NULL;

	hSrcDC = CreateCompatibleDC(0);
	hDstDC = CreateCompatibleDC(0);

	if (hSrcDC == NULL || hDstDC == NULL)
		goto cleanup;

	hNewBitmap = CreateMeABitmap(hDstDC, nAimHeight, nAimHeight, nColorDepth, NULL);

	if (hNewBitmap == NULL)
		goto cleanup;

	hOldSrcSel = SelectObject(hSrcDC, hBitmap);
	hOldDstSel = SelectObject(hDstDC, hNewBitmap);

	if (hOldSrcSel == NULL || hOldDstSel == NULL)
	{
		DeleteObject(hNewBitmap);
		hNewBitmap = NULL;

		goto cleanup;
	}

	const LONG nXOffset = max((nAimHeight-nTargetWidth)/2,0);
	const LONG nYOffset = max((nAimHeight-nTargetHeight)/2,0);

	{
		BLENDFUNCTION bdf = {0};

		bdf.BlendOp = AC_SRC_OVER;
		bdf.SourceConstantAlpha = 255;
		bdf.AlphaFormat = AC_SRC_ALPHA;

		if (!AlphaBlend(	hDstDC, nXOffset, nYOffset, nAimHeight-nXOffset*2, nAimHeight-nYOffset*2, 
					hSrcDC, 0, 0, nWidth, nHeight, bdf))
		{
			DeleteObject(hNewBitmap);
			hNewBitmap = NULL;

			goto cleanup;
		}
	}

cleanup:

	if (hSrcDC != NULL)
	{
		if (hOldSrcSel != NULL)
			SelectObject(hSrcDC, hOldSrcSel);

		DeleteDC(hSrcDC);
	}

	if (hDstDC != NULL)
	{
		if (hOldDstSel != NULL)
			SelectObject(hSrcDC, hOldDstSel);

		DeleteDC(hDstDC);
	}

	return hNewBitmap;
}



HICON	CreateIconFromBitMap(HBITMAP hBitmap, const UINT nAimHeight)
{
	HBITMAP hNewBitmap = CreateBitmapOfHeight(hBitmap, nAimHeight, 32);

	ICONINFO newIcon = {0};

	newIcon.hbmColor = hNewBitmap;
	newIcon.hbmMask = CreateBitmap(nAimHeight,nAimHeight,1,1,NULL);

	assert(newIcon.hbmMask != NULL);

	HICON hIcon = CreateIconIndirect(&newIcon);

	if (hNewBitmap != hBitmap)
		DeleteObject(hNewBitmap);

	DeleteObject(hBitmap);
	DeleteObject(newIcon.hbmMask);

	return hIcon;
}



HBITMAP		ExtractBitmap( const std::wstring& rsFile, LONG /*nWidth*/, LONG nHeight, WORD nDepth)
{
	// Yer, we don't acturally use the width, at the moment this is only used by thumbnails code which asks for squares

	Gdiplus::GdiplusStartupInput GDiplusStartupInput;
	ULONG_PTR nDiplusToken;

	HBITMAP hResult = NULL;

	if (!Gdiplus::GdiplusStartup(&nDiplusToken, &GDiplusStartupInput, NULL) == Gdiplus::Ok)
		return NULL;
	
	Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromFile(rsFile.c_str(), TRUE);

	if (pBitmap == NULL)
	{
		Gdiplus::GdiplusShutdown(nDiplusToken);
		return NULL;
	}

	if (pBitmap->GetLastStatus() != Gdiplus::Ok)
	{
		//Mmmmm ok, maybe it's a icon in some form...

		HICON hIcon = GetSpecificIcon(rsFile.c_str(), 0, nHeight);

		if (hIcon != NULL)
		{
			HDC hDC = CreateCompatibleDC(0);

			if (hDC != NULL)
			{
				hResult = CreateMeABitmap(hDC, nHeight, nHeight, nDepth, NULL);

				if (hResult != NULL)
				{
					HGDIOBJ hOldSel = SelectObject(hDC, hResult);

					if (hOldSel != NULL)
					{
						if (!DrawIconEx(hDC, 0, 0, hIcon, nHeight, nHeight, NULL, NULL, DI_NORMAL))
						{
							 SelectObject(hDC, hOldSel);

							 DeleteObject(hResult);
							 hResult = NULL;
						}
						else SelectObject(hDC, hOldSel);
					}
				}

				DeleteDC(hDC);
			}

			DestroyIcon(hIcon);
		}
	}
	else
	{

		HBITMAP hOrg = NULL;

		if (pBitmap->GetHBITMAP(Gdiplus::Color::Transparent, &hOrg) == Gdiplus::Ok)
		{
			hResult = CreateBitmapOfHeight(hOrg, nHeight, nDepth);

			if (hResult != hOrg)
				DeleteObject(hOrg);
		}
	}

	delete pBitmap;

	Gdiplus::GdiplusShutdown(nDiplusToken);

	return hResult;
}



static bool		GetShortcutIconLocation(std::wstring& rsIconPath, int& rnIndex, LPCWSTR sLinkFile)
{
	CComPtr<IShellLink>		pSL;

	HRESULT hr = pSL.CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER);

	if (FAILED(hr))
		return false;

	CComQIPtr<IPersistFile> pPF(pSL);

	if (pPF.p == NULL)
		return false;

	hr = pPF->Load(sLinkFile, STGM_READ);

	if (FAILED(hr))
		return false;

	WCHAR sBuffer[MAX_PATH];

	hr = pSL->GetIconLocation(sBuffer, MAX_PATH, &rnIndex);

	if (FAILED(hr))
		return false;

	rsIconPath = sBuffer;

	return true;
}


#pragma pack(push)
#pragma pack(2)

struct GRPICONDIRENTRY
{
	BYTE bWidth;                                    // Width, in pixels, of the image
	BYTE bHeight;                                   // Height, in pixels, of the image
	BYTE bColorCount;                               // Number of colors in image (0 if >=8bpp)
	BYTE bReserved;                                 // Reserved ( must be 0)
	WORD wPlanes;                                   // Color Planes
	WORD wBitCount;                                 // Bits per pixel
	DWORD dwBytesInRes;                             // How many bytes in this resource?
	WORD nID;                                       // Resource ID
};

struct GRPICONDIR
{
    WORD			idReserved;                        // Reserved (must be 0)
    WORD			idType;                            // Resource Type (1 for icons)
    WORD			idCount;                           // How many images?
    GRPICONDIRENTRY	idEntries[1];                      // An entry for each image (idCount of 'em)
};
#pragma pack(pop)


//Duplicate of the above is required because of packing and dwImageOffset/nID size difference


struct ICONDIRENTRY
{
	BYTE        bWidth;          // Width, in pixels, of the image
	BYTE        bHeight;         // Height, in pixels, of the image
	BYTE        bColorCount;     // Number of colors in image (0 if >=8bpp)
	BYTE        bReserved;       // Reserved ( must be 0)
	WORD        wPlanes;         // Color Planes
	WORD        wBitCount;       // Bits per pixel
	DWORD       dwBytesInRes;    // How many bytes in this resource?
	DWORD       dwImageOffset;   // Where in the file is this image?
};

typedef std::pair<void*,ICONDIRENTRY>	LOADEDICONDIRENTRY;

struct ICONDIR
{
    WORD           idReserved;   // Reserved (must be 0)
    WORD           idType;       // Resource Type (1 for icons)
    WORD           idCount;      // How many images?
    ICONDIRENTRY   idEntries[1]; // An entry for each image (idCount of 'em)
};


static size_t	GetIconDirSize(WORD idCount)
{
	return GETOFFSET(ICONDIR,idEntries) + (sizeof(ICONDIRENTRY) * (idCount));
}

static UINT		GetIconHeight(BYTE bHeight)
{
	if (bHeight == 0)
		return 256;
	else
		return bHeight;
}


template<typename T>
bool	IsBetterFit(T* pEntry, T* pBestFit, UINT nHeight)
{
	const UINT nBestFitHeight = GetIconHeight(pBestFit->bHeight);
	const UINT nEntryHeight = GetIconHeight(pEntry->bHeight);

	nHeight = GetIconHeight((BYTE)nHeight); //In case unprocessed source

	if (nEntryHeight == nHeight)
	{
		if (nBestFitHeight != nHeight || pEntry->wBitCount > pBestFit->wBitCount)
			return true;
	}
	else if ( (nBestFitHeight > nHeight && nEntryHeight < nBestFitHeight && nEntryHeight > nHeight)
							||
			 (nBestFitHeight < nHeight && nEntryHeight > nBestFitHeight) 
							||
			 (nBestFitHeight == nEntryHeight && pEntry->wBitCount > pBestFit->wBitCount) )
	{
		return true;
	}

	return false;
}


template<typename T, typename T2>
T2* FindBestEntry(T *pIcons, UINT nHeight)
{
	T2* pBestFit = pIcons->idEntries;

	for(WORD n =1; n < pIcons->idCount; ++n)
	{
		T2* pEntry = &pIcons->idEntries[n];

		if (pEntry->bReserved != 0)
			return NULL;

		if (IsBetterFit(pEntry, pBestFit, nHeight))
			pBestFit = pEntry;
	}

	return pBestFit;
}


struct IndexedOfTypePacket
{
	LPWSTR	sName;
	int		nIndex;
	WCHAR	sBuffer[MAX_PATH];
};


BOOL CALLBACK	IndexedOfTypeCB(  HMODULE hModule, LPCWSTR lpszType, LPTSTR lpszName, LONG_PTR lParam)
{
	IndexedOfTypePacket* pPacket = (IndexedOfTypePacket*)lParam;

	if (pPacket->nIndex < 0)
	{
		if (IS_INTRESOURCE(lpszName) && abs(pPacket->nIndex) == *(int*)&lpszName)
		{
			pPacket->sName = lpszName;

			return FALSE;
		}
	}
	else if (pPacket->nIndex-- == 0)
	{
		if (!IS_INTRESOURCE(lpszName))
		{
			wcscpy_s(pPacket->sBuffer, MAX_PATH, lpszName);

			pPacket->sName = pPacket->sBuffer;
		}
		else pPacket->sName = lpszName;

		return FALSE;
	}

	return TRUE;
}


static ICONDIR*	ReadIconDir(HANDLE hFile, size_t* pnSize = NULL)
{
	if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
		return NULL;

	ICONDIR tempIconDir;
	DWORD dwBytesRead = 0;

	// Read the Reserved word
	ReadFile( hFile, &(tempIconDir.idReserved), sizeof( WORD ), &dwBytesRead, NULL );
	// Read the Type word - make sure it is 1 for icons

	if (dwBytesRead != sizeof(WORD) || tempIconDir.idReserved != 0)
		return NULL;

	ReadFile( hFile, &(tempIconDir.idType), sizeof( WORD ), &dwBytesRead, NULL );

	if (dwBytesRead != sizeof(WORD) || tempIconDir.idType != 1)
		return NULL;

	// Read the count - how many images in this file?
	ReadFile( hFile, &(tempIconDir.idCount), sizeof( WORD ), &dwBytesRead, NULL );

	if (dwBytesRead != sizeof(WORD))
		return NULL;

	const size_t nSize = GetIconDirSize(tempIconDir.idCount);
		
	if (pnSize != NULL)
		*pnSize = nSize;

	ICONDIR* pIconDir = (ICONDIR*)malloc(nSize);

	pIconDir->idCount		= tempIconDir.idCount;
	pIconDir->idReserved	= tempIconDir.idReserved;
	pIconDir->idType		= tempIconDir.idType;


	// Read the ICONDIRENTRY elements
	ReadFile( hFile, pIconDir->idEntries, pIconDir->idCount * sizeof(ICONDIRENTRY), &dwBytesRead, NULL );

	if (dwBytesRead != pIconDir->idCount * sizeof(ICONDIRENTRY))
	{
		free(pIconDir);
		pIconDir = NULL;
	}

	return pIconDir;
}


static bool		ReadIconDirFull(const std::wstring& rsFile, std::vector<LOADEDICONDIRENTRY>& rEntries)
{
	HANDLE hFile = CreateFile(rsFile.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == NULL && hFile == INVALID_HANDLE_VALUE)
		return false;

	std::vector<LOADEDICONDIRENTRY> entriesData;

	bool bResult = false;

	size_t nIconDirSize;

	ICONDIR* pIconDir = ReadIconDir(hFile, &nIconDirSize);

	if (pIconDir != NULL)
	{
		bResult = true;

		for(WORD n = 0; n < pIconDir->idCount; ++n)
		{
			ICONDIRENTRY& rEntry = pIconDir->idEntries[n];

			if (rEntry.dwBytesInRes)
			{
				void* pEntryData = malloc(rEntry.dwBytesInRes);

				if (pEntryData)
				{
					DWORD nBytesRead = 0;

					SetFilePointer( hFile, rEntry.dwImageOffset, NULL, FILE_BEGIN );
					ReadFile(hFile, pEntryData, rEntry.dwBytesInRes, &nBytesRead, NULL);

					if (rEntry.dwBytesInRes == nBytesRead)
						rEntries.push_back(LOADEDICONDIRENTRY(pEntryData, rEntry));
					else
						free(pEntryData);
				}
			}
		}

		free(pIconDir);
	}

	CloseHandle(hFile);
	

	return bResult;
}


static bool		WriteWord(HANDLE hFile, WORD nValue)
{
	DWORD nWritten = 0;

	WriteFile(hFile, &nValue, sizeof(WORD), &nWritten, NULL);

	return (nWritten == sizeof(WORD));
}


static bool		WriteIconDirFull(const std::wstring& rsFile, const std::vector<LOADEDICONDIRENTRY>& rEnties)
{
	HANDLE hFile = CreateFile(rsFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == NULL && hFile == INVALID_HANDLE_VALUE)
		return false;

	bool bResult = false;

	if (WriteWord(hFile, 0) && //idReserved
		WriteWord(hFile, 1) && //idType
		WriteWord(hFile, (WORD)rEnties.size()) )  //idCount
	{
		size_t nOffset = GetIconDirSize((WORD)rEnties.size())-sizeof(WORD); //No padding on disc;

		bResult = true;

		for(std::vector<LOADEDICONDIRENTRY>::const_iterator it = rEnties.begin();
			bResult && it != rEnties.end(); ++it)
		{
			ICONDIRENTRY entry = it->second;

			entry.dwImageOffset = (DWORD)nOffset;

			nOffset += entry.dwBytesInRes;

			DWORD nWritten = 0;

			WriteFile(hFile, &entry, sizeof(ICONDIRENTRY), &nWritten, NULL);

			if (nWritten != sizeof(ICONDIRENTRY))
				bResult = false;
		}

		for(std::vector<LOADEDICONDIRENTRY>::const_iterator it = rEnties.begin();
			bResult && it != rEnties.end(); ++it)
		{
			DWORD nWritten = 0;

			WriteFile(hFile, it->first, it->second.dwBytesInRes, &nWritten, NULL);

			if (nWritten != it->second.dwBytesInRes)
				bResult = false;
		}
	}

	CloseHandle(hFile);
		
	return bResult;
}


static HICON	ReadIcon(HANDLE hFile, ICONDIRENTRY* pEntry)
{
	BYTE* pData = (BYTE*)malloc( pEntry->dwBytesInRes );

	if (pData == NULL)
		return NULL;

	DWORD dwBytesRead = 0;

	// Read the image data
	SetFilePointer( hFile, pEntry->dwImageOffset, NULL, FILE_BEGIN );
	ReadFile( hFile, pData, pEntry->dwBytesInRes, &dwBytesRead, NULL );

	HICON hResult = NULL;

	if (pEntry->dwBytesInRes == dwBytesRead)
		hResult = CreateIconFromResourceEx(pData, dwBytesRead, TRUE, 0x00030000,GetIconHeight(pEntry->bWidth), GetIconHeight(pEntry->bHeight), LR_DEFAULTCOLOR); 

	free( pData );

	return hResult;
}


static HICON	FindBestIconFromIco( LPCWSTR sFile, UINT nHeight)
{
	HICON hResult = NULL;

	HANDLE hFile = CreateFile(sFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
		return NULL;

	size_t nIconDirSize;

	ICONDIR* pIconDir = ReadIconDir(hFile, &nIconDirSize);

	if (pIconDir == NULL)
		goto cleanup;

	ICONDIRENTRY* pBestFit = FindBestEntry<ICONDIR, ICONDIRENTRY>(pIconDir, nHeight);

	if (pBestFit == NULL)
		goto cleanup;

	hResult = ReadIcon(hFile, pBestFit);
	
cleanup:

	CloseHandle(hFile);

	if (pIconDir != NULL)
		free(pIconDir);

	return hResult;
}


//Read "Icons in Win32"  http://msdn.microsoft.com/en-us/library/ms997538.aspx and the wine source of CreateIconFromBMI
static bool		CreateIconStream(void*& rpData, size_t& rnDataSize, LONG& rnWidth, LONG& rnHeight, HICON hIcon)
{
	BITMAPINFO			bmi		= {0};

	bool bResult = false;

	ICONINFO ii = {0};

	if (!GetIconInfo(hIcon, &ii))
		return false;

	BITMAP bm;

	if (!GetObject(ii.hbmColor, sizeof(bm), &bm))
		goto error;

	if (bm.bmBitsPixel != 32)
	{
		HDC hDC = CreateCompatibleDC(0);

		if (hDC == NULL)
			goto error;

		LONG nHeight = bm.bmHeight;

		HBITMAP h32Version = CreateAlphaBitmap(hDC, nHeight, nHeight, NULL);

		if (h32Version == NULL)
			goto error;

		HGDIOBJ hOld = SelectObject(hDC, h32Version);

		BOOL bRendered = DrawIcon(hDC, 0, 0, hIcon);//DrawIconEx(hDC, 0, 0, hIcon, nHeight, nHeight, 0, NULL, DI_NORMAL );

		SelectObject(hDC, hOld);
		DeleteDC(hDC);

		if (!bRendered)
		{
			DeleteObject(h32Version);
	
			goto error;
		}

		DeleteObject(ii.hbmColor);
		ii.hbmColor = h32Version;		
	}
	else
	{
		BITMAP bmMask;

		if (!GetObject(ii.hbmMask, sizeof(bmMask), &bmMask))
			goto error;

		if (bmMask.bmBitsPixel != 1)
			goto error; //What the crap?
	}


	LONG nColorImageSize = bm.bmHeight * bm.bmWidthBytes; //32bpp
	LONG nMaskImageSize = bm.bmHeight * bm.bmWidth; //1bpp

	rnDataSize = sizeof(BITMAPINFOHEADER) + nColorImageSize + nMaskImageSize;
	rpData = (BYTE*)malloc(rnDataSize);

	if (rpData == NULL)
		goto error;

	ZeroMemory(rpData, rnDataSize);

	size_t nPos = 0;

	rnWidth = bm.bmWidth;
	rnHeight = bm.bmHeight;

	BITMAPINFOHEADER * pHeader = (BITMAPINFOHEADER*)rpData;

	pHeader->biSize		= sizeof(BITMAPINFOHEADER);
	pHeader->biWidth	= rnWidth;
	pHeader->biHeight	= (rnHeight*2); //Don't know why it must be doubled
	pHeader->biPlanes	= 1;
	pHeader->biBitCount	= 32;

	nPos += pHeader->biSize;

	//No pallete to add

	
	//Add XOR/Color image

	HDC hDC = CreateCompatibleDC(0);

	if (hDC == NULL)
		goto error;

	HGDIOBJ hOld = SelectObject(hDC, ii.hbmColor);


	bmi.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biPlanes		= 1;
	bmi.bmiHeader.biCompression	= BI_RGB;
	bmi.bmiHeader.biWidth		= bm.bmWidth;
	bmi.bmiHeader.biHeight		= bm.bmHeight;
	bmi.bmiHeader.biBitCount	= 32;
	
	GetDIBits(hDC, ii.hbmColor, 0, bm.bmHeight, &((BYTE*)rpData)[nPos], &bmi, DIB_RGB_COLORS);

	nPos += nColorImageSize;

	//No need to add mask, it's all 0 as it's a 32bit icon

	SelectObject(hDC, hOld);
	DeleteDC(hDC);

	bResult = true;

cleanup:

	if (ii.hbmMask)
		DeleteObject(ii.hbmMask);

	if (ii.hbmColor)
		DeleteObject(ii.hbmColor);

	return bResult;

error:

	if (rpData != NULL)
	{
		free(rpData);
		rpData = NULL;
	}

	goto cleanup;
}





bool			WriteToIconFile(const std::wstring& rsFile, HICON hIcon, bool bAppend)
{
	return WriteToIconFile(rsFile, &hIcon, 1, bAppend);
}


bool			WriteToIconFile(const std::wstring& rsFile, const HICON* phIcons, int nIconNum, bool bAppend)
{
	std::vector<LOADEDICONDIRENTRY> entriesData;

	bool bResult = false;

	if (bAppend)
		ReadIconDirFull(rsFile, entriesData);

	while(nIconNum--)
	{
		LONG nWidth, nHeight;
		void* pData;
		size_t nDataSize = 0;

		if (CreateIconStream(pData, nDataSize, nWidth, nHeight, *phIcons))
		{
			ICONDIRENTRY entry = {0};

			entry.bWidth = (BYTE)nWidth;
			entry.bHeight = (BYTE)nHeight;
			entry.dwBytesInRes = (DWORD)nDataSize;
			entry.wBitCount = 32;

			entriesData.push_back(LOADEDICONDIRENTRY(pData, entry));
		}

		++phIcons;
	}

	bResult = WriteIconDirFull(rsFile, entriesData);


	for(size_t n = 0; n < entriesData.size(); ++n)
		free(entriesData[n].first);

	return bResult;
}



static HICON	LoadIcon(HMODULE hModule, DWORD nIconID)
{
    HRSRC hRsrc = FindResource(hModule, MAKEINTRESOURCE(nIconID), RT_ICON);

    if (hRsrc == NULL)
		return NULL;

    HGLOBAL hGlobal = LoadResource(hModule, hRsrc);

	HICON hResult = NULL;

    if (hGlobal)
    {
        BYTE *pData = reinterpret_cast<BYTE*>(LockResource(hGlobal));

		size_t nSize = SizeofResource(hModule, hRsrc);

		BITMAPINFOHEADER* pHeader = (BITMAPINFOHEADER*)pData;

		hResult = CreateIconFromResourceEx(pData, (DWORD)nSize, TRUE, 0x00030000, pHeader->biWidth, pHeader->biHeight/2, LR_DEFAULTCOLOR); 

        FreeResource(hGlobal);
	}
	
	return hResult;
}


static HICON	FindBestIconFromExe( LPCWSTR sFile, int nIndex, UINT nHeight)
{
	HMODULE hModule = LoadLibraryEx(sFile, NULL, LOAD_LIBRARY_AS_DATAFILE);

	HICON hIcon = NULL;

	if (hModule == NULL)
		return FindBestIconFromIco(sFile, nHeight);

	
	IndexedOfTypePacket packet = {NULL, nIndex};

	EnumResourceNames(hModule, RT_GROUP_ICON, IndexedOfTypeCB, (LONG_PTR)&packet);

	if (packet.sName != NULL)
	{
		HRSRC hResource = FindResource(hModule, packet.sName, RT_GROUP_ICON);

		if (hResource != NULL)
		{
			HGLOBAL hGlob = LoadResource(hModule, hResource);

			if (hGlob != NULL)
			{
				BYTE* pData = (BYTE*)LockResource(hGlob);

				if (pData != NULL)
				{
					//Could use FindBestEntry but this is does the same job
					int nIconID = LookupIconIdFromDirectoryEx(pData, TRUE, nHeight, nHeight, LR_DEFAULTCOLOR);

					hIcon = LoadIcon(hModule, nIconID);
				}

				FreeResource(hGlob);
			}
		}
	}

	FreeLibrary(hModule);
	

	return hIcon;
}


struct IconAndInfo
{
	HICON hIcon;
	BYTE bWidth;
	BYTE bHeight;
	WORD wBitCount;
};


static bool		GenericReadIconDir(const std::wstring& rsFile, int nIndex, std::vector<IconAndInfo>& rEntries)
{
	HMODULE hModule = LoadLibraryEx(rsFile.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE);

	bool bResult = false;

	if (hModule == NULL)
	{
		HICON hResult = NULL;

		HANDLE hFile = CreateFile(rsFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
			return false;

		size_t nIconDirSize;

		ICONDIR* pIconDir = ReadIconDir(hFile, &nIconDirSize);

		if (pIconDir != NULL)
		{
			for(WORD n = 0; n < pIconDir->idCount; ++n)
			{
				ICONDIRENTRY* pEntry = &pIconDir->idEntries[n];
				IconAndInfo entry;

				entry.hIcon = ReadIcon(hFile, pEntry);

				if (entry.hIcon != NULL)
				{
					entry.bHeight	= pEntry->bHeight;
					entry.bWidth	= pEntry->bWidth;
					entry.wBitCount = pEntry->wBitCount;

					rEntries.push_back(entry);
					bResult = true;
				}
			}

			free(pIconDir);
		}

		CloseHandle(hFile);

		return bResult;
	}


	IndexedOfTypePacket packet = {NULL, nIndex};

	EnumResourceNames(hModule, RT_GROUP_ICON, IndexedOfTypeCB, (LONG_PTR)&packet);

	if (packet.sName != NULL)
	{
		HRSRC hResource = FindResource(hModule, packet.sName, RT_GROUP_ICON);

		if (hResource != NULL)
		{
			HGLOBAL hGlob = LoadResource(hModule, hResource);

			if (hGlob != NULL)
			{
				BYTE* pData = (BYTE*)LockResource(hGlob);

				if (pData != NULL)
				{
					GRPICONDIR* pIconDir = (GRPICONDIR*)pData;

					for(WORD n = 0; n < pIconDir->idCount; ++n)
					{
						GRPICONDIRENTRY* pEntry = &pIconDir->idEntries[n];
						IconAndInfo entry;

						entry.hIcon = LoadIcon(hModule, pEntry->nID);

						if (entry.hIcon != NULL)
						{
							entry.bHeight	= pEntry->bHeight;
							entry.bWidth	= pEntry->bWidth;
							entry.wBitCount = pEntry->wBitCount;

							rEntries.push_back(entry);
							bResult = true;
						}
					}
				}

				FreeResource(hGlob);
			}
		}
	}

	FreeLibrary(hModule);

	return bResult;

	/*
	size_t nDot = rsFile.rfind(L'.');

	if (nDot == -1)
		return false;

	LPCWSTR sExt = rsFile.c_str()+nDot;

	if (ExtIsIconFile(sExt))
		return ReadIconDirFull(rsFile, rEntries);
	
	if (!ExtIsCodeImage(sExt))
		return false;*/





}


BOOL CALLBACK	CountOfTypeCB(  HMODULE , LPCTSTR , LPTSTR , LONG_PTR lParam)
{
	(*((ULONG*)lParam))++;

	return TRUE;
}







int				AddIconToResourcedFile(const std::wstring& rsFile, HICON hIcon)
{
	ULONG nIconNum		= 0;
	ULONG nIconGroupNum	= 0;

	HMODULE hModule = LoadLibraryEx(rsFile.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE);

	if (hModule != NULL && hModule != INVALID_HANDLE_VALUE)
	{
		EnumResourceNames(hModule, RT_ICON, CountOfTypeCB, (LONG_PTR)&nIconNum);
		EnumResourceNames(hModule, RT_GROUP_ICON, CountOfTypeCB, (LONG_PTR)&nIconGroupNum);

		FreeLibrary(hModule);
	}

	HANDLE hHandle = BeginUpdateResource(rsFile.c_str(), FALSE);

	if (hHandle == NULL || hHandle == INVALID_HANDLE_VALUE)
		return -1;

	int nResult = -1;

	void* pData;
	size_t nDataSize;
	LONG nWidth;
	LONG nHeight;

	if (CreateIconStream(pData, nDataSize, nWidth, nHeight, hIcon))
	{
		//Add to resource
		UpdateResource(hHandle, RT_ICON, MAKEINTRESOURCE(nIconNum), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), pData, (DWORD)nDataSize);
		free(pData);

		size_t nIconDirSize = (sizeof(WORD) * 3) + (sizeof(ICONDIRENTRY) * 1);

		GRPICONDIR* pIconDir = (GRPICONDIR*)malloc( nIconDirSize );

		ZeroMemory(pIconDir, nIconDirSize);

		pIconDir->idType		= 1;
		pIconDir->idCount		= 1;

		GRPICONDIRENTRY* pEntry = &pIconDir->idEntries[0];

		pEntry->bHeight			= (BYTE)nWidth;
		pEntry->bWidth			= (BYTE)nHeight;
		pEntry->dwBytesInRes	= (DWORD)nDataSize;
		pEntry->nID				= (WORD)nIconNum;
		pEntry->wBitCount		= 32;
		pEntry->wPlanes			= 1;


		pData = (BYTE*)pIconDir;
		nDataSize = nIconDirSize;

		//nIconGroupNum need one adding because it's one based not zero based
		UpdateResource(hHandle, RT_GROUP_ICON, MAKEINTRESOURCE(nIconGroupNum+1), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), pData, (DWORD)nDataSize);

		free(pData);

		nResult = nIconGroupNum;
	}

	EndUpdateResource(hHandle, FALSE);

	return nResult;
}



static bool		ExtractIconInfoFromKey( std::wstring& rsIconFilePath, int& rnIconIndex, HKEY hKey)
{
	HKEY hDefaultIconKey;

	if (RegOpenKeyEx(hKey, L"DefaultIcon", 0, KEY_READ, &hDefaultIconKey) != ERROR_SUCCESS)
			return false;

	rsIconFilePath.resize(0);

	LSTATUS nStat = ERROR_MORE_DATA;
	DWORD nValueBufferSize = 0;

	while(nStat == ERROR_MORE_DATA)
	{
		rsIconFilePath.assign(rsIconFilePath.length()+MAX_PATH, L' ');

		nValueBufferSize = (DWORD)rsIconFilePath.length() * sizeof(WCHAR);

		nStat = RegQueryValueEx(hDefaultIconKey, NULL, NULL, NULL, (LPBYTE)rsIconFilePath.c_str(), &nValueBufferSize);
	}

	bool bResult = false;

	//Instead of PathParseIconLocation
	if (nStat == ERROR_SUCCESS)
	{
		size_t nDevider = rsIconFilePath.find(L',');
			
		if (nDevider != -1)
		{
			rnIconIndex = _wtoi(&rsIconFilePath.c_str()[nDevider+1]);
			rsIconFilePath.resize(nDevider);
		}
		else
		{
			rsIconFilePath.resize((nValueBufferSize/sizeof(WCHAR))-1);
			rnIconIndex = 0;
		}

		bResult = true;
	}

	RegCloseKey(hDefaultIconKey);

	return bResult;	
}


static bool		ExtractIconInfoForExtFromSubKey(std::wstring& rsIconFilePath, int& rnIconIndex, HKEY hKey, LPCTSTR sSubKey)
{
	bool bFound = ExtractIconInfoFromKey(rsIconFilePath, rnIconIndex, hKey);

	WCHAR sValue[MAX_PATH*2];
	DWORD nType = REG_SZ;
	DWORD nValueBufferSize = sizeof(sValue);

	if (RegQueryValueEx(hKey, sSubKey, NULL, &nType, (LPBYTE)sValue, &nValueBufferSize) == ERROR_SUCCESS)
	{
		HKEY hTypeKey = NULL;

		if (RegOpenKeyEx(HKEY_CLASSES_ROOT, sValue, 0, KEY_READ , &hTypeKey) == ERROR_SUCCESS)
		{
			bFound = ExtractIconInfoFromKey(rsIconFilePath, rnIconIndex, hTypeKey);

			RegCloseKey(hTypeKey);
		}

		if (!bFound)
		{
			HKEY hSysTypeKey = NULL;

			if (RegOpenKeyEx(HKEY_CLASSES_ROOT, (std::wstring(L"SystemFileAssociations\\") + sValue).c_str(), 0, KEY_READ , &hSysTypeKey) == ERROR_SUCCESS)
			{
				bFound = ExtractIconInfoFromKey(rsIconFilePath, rnIconIndex, hSysTypeKey);

				RegCloseKey(hSysTypeKey);
			}
		}
	}

	return bFound;
}



//Instead of SHGetFileInfo so there is no MAX_PATH limit, and works more often!
bool			ExtractIconInfoForExt( std::wstring& rsIconFilePath, int& rnIconIndex, LPCWSTR sExt )
{
	if (sExt == NULL)
		return false;

	HKEY hKey;

	std::wstring sIconFilePath;
	bool bFound = false;

	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, sExt, 0, KEY_READ , &hKey) == ERROR_SUCCESS)
	{
		bFound = ExtractIconInfoForExtFromSubKey(sIconFilePath, rnIconIndex, hKey, NULL);

		if (!bFound)
			bFound = ExtractIconInfoForExtFromSubKey(sIconFilePath, rnIconIndex, hKey, L"PerceivedType");

		RegCloseKey(hKey);
	}


	if (!bFound && RegOpenKeyEx(HKEY_CLASSES_ROOT, (std::wstring(L"SystemFileAssociations\\") + sExt).c_str(), 0, KEY_READ , &hKey) == ERROR_SUCCESS)
	{
		bFound = ExtractIconInfoForExtFromSubKey(sIconFilePath, rnIconIndex, hKey, NULL);

		if (!bFound)
			bFound = ExtractIconInfoForExtFromSubKey(sIconFilePath, rnIconIndex, hKey, L"PerceivedType");

		RegCloseKey(hKey);
	}	

	if (bFound)
	{
		DWORD nSize = ExpandEnvironmentStrings(sIconFilePath.c_str(), NULL, 0);

		rsIconFilePath.resize(nSize);

		ExpandEnvironmentStrings(sIconFilePath.c_str(), (LPWSTR)rsIconFilePath.c_str(), (DWORD)rsIconFilePath.length());
		
		rsIconFilePath.resize(nSize-1);

		EnsureWin32Path(rsIconFilePath);

		EnsureFullPath(rsIconFilePath);

		boost::trim_right_if(rsIconFilePath, boost::is_any_of(L"\""));
		boost::trim_left_if(rsIconFilePath, boost::is_any_of(L"\""));

		return true;
	}
	else return false;
}


HICON			GetSpecificIcon( const std::wstring& rsFile, const int nIndex, const UINT nHeight)
{
	HICON hIcon = NULL;

	size_t nDot = rsFile.rfind(L'.');

	if (nDot != -1)
	{
		LPCWSTR sExt = &rsFile[nDot];

		if (ExtIsCodeImage(sExt))
		{
			//LoadImage and ExtractIcon will scale, not just find the closed in size
			//hIcon = ExtractIcon(g_hInstance, rsFile.c_str(), nIndex);
			hIcon = FindBestIconFromExe(rsFile.c_str(), nIndex, nHeight);
		}
		else if (ExtIsIconFile(sExt))
		{
			//LoadImage and ExtractIcon will scale, not just find the closed in size
			//hIcon = (HICON)LoadImage(g_hInstance, rsFile.c_str(), IMAGE_ICON, nHeight, nHeight, LR_LOADFROMFILE);
			//hIcon = ExtractIcon(g_hInstance, rsFile.c_str(), nIndex);	
			hIcon = FindBestIconFromIco(rsFile.c_str(), nHeight);
		}
		else if (ExtIsShortcut(sExt))
		{
			std::wstring sTarget;
			int nIndex;

			if (GetShortcutIconLocation(sTarget, nIndex, rsFile.c_str()))
				hIcon = GetSpecificIcon(sTarget, nIndex, nHeight);
			else
				hIcon = ExtractIcon(g_hInstance, rsFile.c_str(), nIndex);
		}
		else
		{
			hIcon = ExtractIconFromImageListFile(rsFile, (UINT)nIndex, nHeight);
		}
	}

	//Last try to find some icon for this!
	if (hIcon == NULL)
		hIcon = ExtractIcon(g_hInstance, rsFile.c_str(), nIndex);

	return hIcon;
}


static void		ManualMaskBlitTo32Alphaed(HDC hDC, DWORD *pDstPixels, const LONG nWidth, const LONG nHeight, const LONG nX, const LONG nY, HBITMAP hColor, HBITMAP hMask)
{
	if (hMask == NULL || hColor == NULL)
		return;

	BITMAP bm;

	GetObject(hColor, sizeof(bm), &bm);

	BITMAPINFO bmi = {0};

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;

	bmi.bmiHeader.biWidth = bm.bmWidth;
	bmi.bmiHeader.biHeight = bm.bmHeight;
	bmi.bmiHeader.biBitCount = 32;

	HANDLE hHeap = GetProcessHeap();

	DWORD* pMaskPixels = (DWORD*)HeapAlloc(hHeap, 0, bmi.bmiHeader.biWidth * 4 * bmi.bmiHeader.biHeight);

	if (pMaskPixels != NULL)
	{
		if (GetDIBits(hDC, hMask, 0, bm.bmHeight, pMaskPixels, &bmi, DIB_RGB_COLORS) ==  bm.bmHeight)
		{
			DWORD* pSrcPixels = (DWORD*)HeapAlloc(hHeap, 0, bmi.bmiHeader.biWidth * 4 * bmi.bmiHeader.biHeight);

			if (pSrcPixels != NULL)
			{
				if (GetDIBits(hDC, hColor, 0, bm.bmHeight, pSrcPixels, &bmi, DIB_RGB_COLORS) ==  bm.bmHeight)
				{
					bool bHasAlpha = false;

					for (LONG n = 0; n < bm.bmHeight*bm.bmWidth; ++n)
						if (pSrcPixels[n] & 0xFF000000)
						{
							bHasAlpha = true;
							break;
						}

					if (bHasAlpha)
					{
						for (LONG y = bm.bmHeight-1, y2 = nHeight-1-nY; y>=0 && y2>=0 ; --y,--y2)
							for (LONG x = bm.bmWidth-1, x2 = nWidth-1-nX; x>=0 && x2>=0 ; --x, --x2)
							{
								const int nSrcIndex = x+y*bm.bmWidth;

								if (!pMaskPixels[nSrcIndex])
								{
									const DWORD nSrcPixel = pSrcPixels[nSrcIndex];

									DWORD& rnDstPixel = pDstPixels[y2*nWidth + x2];

									const DWORD nDstAlpha = (rnDstPixel>>24)&0xFF;

									if (nDstAlpha != 0)
									{
										const DWORD nSrcAlpha = (nSrcPixel>>24)&0xFF;

										if (nSrcAlpha != 0)
										{
											if (nSrcAlpha != 255)
											{
												const DWORD nSrcInvAlpha = 255-nSrcAlpha;

												const DWORD nSrcR = ((((nSrcPixel>>16)&0xFF)*nSrcAlpha)>>8)&0xFF;
												const DWORD nSrcG = ((((nSrcPixel>>8)&0xFF)*nSrcAlpha)>>8)&0xFF;
												const DWORD nSrcB = (((nSrcPixel&0xFF)*nSrcAlpha)>>8)&0xFF;

												const DWORD nDstR = ((((rnDstPixel>>16)&0xFF)*nSrcInvAlpha)>>8)&0xFF;
												const DWORD nDstG = ((((rnDstPixel>>8)&0xFF)*nSrcInvAlpha)>>8)&0xFF;
												const DWORD nDstB = (((rnDstPixel&0xFF)*nSrcInvAlpha)>>8)&0xFF;

												rnDstPixel =	min(nSrcAlpha + nDstAlpha, 255)<<24 | 
																min(nSrcR + nDstR, 255)<<16 |
																min(nSrcG + nDstG, 255)<<8 |
																min(nSrcB + nDstB, 255);
											}
											else rnDstPixel = nSrcPixel;
										}
									}
									else rnDstPixel = nSrcPixel;
								}
							}
					}
					else
					{
						for (LONG y = bm.bmHeight-1, y2 = nHeight-1-nY; y>=0 && y2>=0 ; --y,--y2)
							for (LONG x = bm.bmWidth-1, x2 = nWidth-1-nX; x>=0 && x2>=0 ; --x, --x2)
							{
								const int nSrcIndex = x+y*bm.bmWidth;

								if (!pMaskPixels[nSrcIndex])
									pDstPixels[y2*nWidth + x2] = 0xFF000000 | pSrcPixels[nSrcIndex];
							}
					}
				}

				HeapFree(hHeap, 0, pSrcPixels);
			}
		}

		HeapFree(hHeap, 0, pMaskPixels);
	}
}



static void		ManualIconBlitTo32Alphaed(HDC hDC, DWORD *pDstPixels, const LONG nWidth, const LONG nHeight, const LONG nX, const LONG nY, HICON hIcon, ICONINFO* pInfo = NULL)
{
	assert(hDC != 0 && pDstPixels != NULL);

	ICONINFO iconInfo = {0};

	if (pInfo == NULL)
	{
		if (!GetIconInfo(hIcon, &iconInfo))
			return;

		pInfo = &iconInfo;
	}

	ManualMaskBlitTo32Alphaed(hDC, pDstPixels, nWidth, nHeight, nX, nY, pInfo->hbmColor, pInfo->hbmMask);

	if (iconInfo.hbmColor != NULL)
		DeleteObject(iconInfo.hbmColor);

	if (iconInfo.hbmMask != NULL)
		DeleteObject(iconInfo.hbmMask);
}





HICON			EnsureIconSizeResolution(HICON hIcon, LONG nHeight)
{
	if (hIcon == NULL)
		return NULL;
	
	ICONINFO iconinfo;

	if (GetIconInfo(hIcon, &iconinfo))
	{
		BITMAP bm;

		GetObject(iconinfo.hbmColor, sizeof(bm), &bm);

		if (bm.bmHeight < nHeight)
		{
			HDC hDC = CreateCompatibleDC(0);

			ICONINFO newIcon = {0};

			DWORD* pBits = NULL;

			newIcon.hbmColor = CreateAlphaBitmap(hDC, nHeight, nHeight, &pBits);
			newIcon.hbmMask = CreateBitmap(nHeight,nHeight,1,1,NULL);

			HGDIOBJ hSel = SelectObject(hDC, newIcon.hbmColor);

			UINT nOffset = (bm.bmHeight < nHeight)?(nHeight-bm.bmHeight)/2:0;
			
			ManualIconBlitTo32Alphaed(hDC, pBits, nHeight, nHeight, nOffset, nOffset, hIcon, &iconinfo);

			DestroyIcon(hIcon);

			hIcon = CreateIconIndirect(&newIcon);

			DeleteObject(newIcon.hbmColor);
			DeleteObject(newIcon.hbmMask);

			DeleteDC(hDC);
		}

		DeleteObject(iconinfo.hbmColor);
		DeleteObject(iconinfo.hbmMask);

	}

	return hIcon;
}


HICON			BurnTogether(HICON hFirst, POINT &rFirst, HICON hSecond, POINT &rSecond, UINT nHeight, bool bOutline )
{
	HICON hResult = NULL;
	ICONINFO newIcon = {0};

	HDC hMemDC = CreateCompatibleDC(0);
	HGDIOBJ hOld = NULL;

	if (hMemDC == NULL)
		goto cleanup;

	DWORD *pBits = NULL;
		
	newIcon.fIcon = TRUE;
	newIcon.hbmColor = CreateAlphaBitmap(hMemDC, nHeight, nHeight, &pBits);
	
	if (newIcon.hbmColor == NULL)
		goto cleanup;

	newIcon.hbmMask = CreateBitmap(nHeight,nHeight,1,1,NULL);

	if (newIcon.hbmMask == NULL)
		goto cleanup;

	hOld = SelectObject(hMemDC, newIcon.hbmColor);

	ManualIconBlitTo32Alphaed(hMemDC, pBits, nHeight, nHeight, rFirst.x, rFirst.y, hFirst);

	if (bOutline)
	{
		for(UINT y = 0; y < nHeight; ++y)
		{
			if (y == 0 || y == nHeight-1)
			{
				for(UINT x = 0; x < nHeight; ++x)
					pBits[x+y*nHeight] = 0xFFE0E0E0;
			}
			else
			{
				pBits[y*nHeight] = 0xFFE0E0E0;
				pBits[nHeight-1+y*nHeight] = 0xFFE0E0E0;
			}
		}
	}

	ManualIconBlitTo32Alphaed(hMemDC, pBits, nHeight, nHeight, rSecond.x, rSecond.y, hSecond);

	hResult = CreateIconIndirect(&newIcon);

cleanup:

	if (newIcon.hbmColor != NULL)
		DeleteObject(newIcon.hbmColor);

	if (newIcon.hbmMask != NULL)
		DeleteObject(newIcon.hbmMask);

	if (hOld != NULL)
		SelectObject(hMemDC, hOld);

	if (hMemDC != NULL)
		DeleteDC(hMemDC);

	return hResult;
}


HICON			ExtractIconFromImageListFile(const std::wstring& rsFile, const UINT nIndex, const UINT nAimHeight)
{
	HICON hIcon = NULL;

	Gdiplus::GdiplusStartupInput GDiplusStartupInput;
	ULONG_PTR nDiplusToken;

	Gdiplus::GdiplusStartup(&nDiplusToken, &GDiplusStartupInput, NULL);

	Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromFile(rsFile.c_str(), TRUE);

	if (pBitmap != NULL)
	{
		const UINT nHeight = pBitmap->GetHeight();
		const UINT nWidth = pBitmap->GetWidth();

		if (nHeight != 0 && (nWidth%nHeight) == 0)
		{
			const UINT nIconNum = nWidth/nHeight;

			if (nIndex < nIconNum)
			{
				if (nIconNum > 1)
				{
					Gdiplus::Bitmap* pSubBitmap = pBitmap->Clone(nIndex*nHeight, 0, nHeight, nHeight, PixelFormat32bppARGB);

					delete pBitmap;
					pBitmap = pSubBitmap;
				}

				if (pBitmap != NULL)
				{
					ICONINFO newIcon = {0};

					newIcon.fIcon = TRUE;

					//GetHICON was causing funny alpha issues on some files. The alpha wasn't in the color or mask bitmap from GetIconInfo...
					pBitmap->GetHBITMAP(Gdiplus::Color::Transparent, &newIcon.hbmColor);

					assert(newIcon.hbmColor != NULL);

					HBITMAP hScaled = CreateBitmapOfHeight(newIcon.hbmColor, nAimHeight, 32);

					if (hScaled != newIcon.hbmColor)
					{
						DeleteObject(newIcon.hbmColor);
						newIcon.hbmColor = hScaled;
					}

					newIcon.hbmMask = CreateBitmap(nAimHeight,nAimHeight,1,1,NULL);

					assert(newIcon.hbmMask != NULL);
					assert(newIcon.hbmColor != NULL);

					hIcon = CreateIconIndirect(&newIcon);

					DeleteObject(newIcon.hbmColor);
					DeleteObject(newIcon.hbmMask);
				}
			}
		}
		else
		{
			//Best I can do at this point
			HBITMAP hBitmap = NULL;

			pBitmap->GetHBITMAP(Gdiplus::Color(), &hBitmap);

			hIcon = CreateIconFromBitMap(hBitmap, nAimHeight);
		}

		if (pBitmap != NULL)
			delete pBitmap;
	}

	Gdiplus::GdiplusShutdown(nDiplusToken);

	return hIcon;
}


HIMAGELIST		LoadImageList(const std::wstring& rsFile, const UINT nAimHeight)
{
	if (rsFile.find(L".bmp") != -1)
	{
		return ImageList_LoadImage(	NULL, rsFile.c_str(), nAimHeight ,0,
									CLR_DEFAULT, IMAGE_BITMAP,
									LR_CREATEDIBSECTION|LR_LOADFROMFILE|LR_LOADTRANSPARENT);
	}
	else
	{
		HIMAGELIST hImageList = NULL;

		Gdiplus::GdiplusStartupInput GDiplusStartupInput;
		ULONG_PTR nDiplusToken;

		Gdiplus::GdiplusStartup(&nDiplusToken, &GDiplusStartupInput, NULL);

		Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromFile(rsFile.c_str(), TRUE);

		if (pBitmap != NULL)
		{
			UINT nHeight = pBitmap->GetHeight();
			UINT nWidth = pBitmap->GetWidth();

			hImageList = ImageList_Create(nAimHeight, nAimHeight, ILC_COLOR32|ILC_MASK, 0, 4);

			UINT nIconNum = nWidth/nHeight;

			if (nIconNum > 0)
			{
				UINT nPos = 0;

				ICONINFO newIcon = {0};

				newIcon.fIcon = TRUE;
				newIcon.hbmMask = CreateBitmap(nHeight,nHeight,1,1,NULL);

				while(nIconNum--)
				{
					HICON hIcon = NULL;

					Gdiplus::Bitmap* pSubBitmap = pBitmap->Clone(nPos, 0, nHeight, nHeight, PixelFormat32bppARGB);

					nPos += nHeight;

					if (pSubBitmap != NULL)
					{
						//GetHICON was causing funny alpha issues on some files. The alpha wasn't in the color or mask bitmap from GetIconInfo...
						pSubBitmap->GetHBITMAP(Gdiplus::Color::Transparent, &newIcon.hbmColor);

						hIcon = CreateIconIndirect(&newIcon);

						DeleteObject(newIcon.hbmColor);
						newIcon.hbmColor = NULL;

						delete pSubBitmap;
					}						

					const int nIndex = ImageList_GetImageCount(hImageList);

					if ( ImageList_SetImageCount(hImageList, nIndex+1) && hIcon != NULL)
						ImageList_ReplaceIcon(hImageList, nIndex, hIcon);

					if (hIcon != NULL)
						DestroyIcon(hIcon);
				}


				DeleteObject(newIcon.hbmMask);
			}

			delete pBitmap;
		}
		
		Gdiplus::GdiplusShutdown(nDiplusToken);

		return hImageList;
	}
}


HICON			LoadAsIcon(const std::wstring& rsPath, const UINT nAimHeight)
{
	HBITMAP hBitmap = NULL;
	
	if (rsPath.length() > 4 &&
		(rsPath[rsPath.length()-4] == L'.') &&
		(tolower(rsPath[rsPath.length()-3]) == L'b') &&
		(tolower(rsPath[rsPath.length()-2]) == L'm') &&
		(tolower(rsPath[rsPath.length()-1]) == L'p') )
	{
		hBitmap = (HBITMAP)LoadImage(g_hInstance, rsPath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION );
	}

	if (hBitmap == NULL)
	{
		Gdiplus::GdiplusStartupInput GDiplusStartupInput;
		ULONG_PTR nDiplusToken;

		Gdiplus::GdiplusStartup(&nDiplusToken, &GDiplusStartupInput, NULL);

		Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromFile(rsPath.c_str(), TRUE);

		if (pBitmap != NULL)
		{
			pBitmap->GetHBITMAP(Gdiplus::Color(), &hBitmap);

			delete pBitmap;

			Gdiplus::GdiplusShutdown(nDiplusToken);
		}

		if (hBitmap == NULL)
			return NULL;
	}

	return CreateIconFromBitMap(hBitmap, nAimHeight);
}


HBITMAP			ExtractAsBitmap(HINSTANCE hExeHandle, LPCWSTR sName, LPCWSTR sType, UINT* pnHeight)
{
	assert(hExeHandle != NULL);
	assert(sName != NULL);
	assert(sType != NULL);

	HBITMAP hResult = NULL;

	HRSRC hRes = FindResourceW(hExeHandle, sName, sType);

	if (hRes != NULL)
	{
		DWORD nSize = SizeofResource(NULL, hRes);

		HGLOBAL hReallyStaticMem = LoadResource(hExeHandle, hRes);

		if (hReallyStaticMem != NULL)
		{
			PVOID pData = LockResource(hReallyStaticMem);

			if (pData != NULL)
			{
				IStream* pStream = NULL;

				//The HGLOBAL from LoadResource isn't a real one, and CreateStreamOnHGlobal need a real one.
				HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, nSize);

				void* pBuffer = ::GlobalLock(hGlobal);

				CopyMemory(pBuffer, pData, nSize);

				CreateStreamOnHGlobal(hGlobal, FALSE, &pStream);

				if (pStream != NULL)
				{
					Gdiplus::GdiplusStartupInput	GDiplusStartupInputData;
					ULONG_PTR						nDiplusToken;

					Gdiplus::GdiplusStartup(&nDiplusToken, &GDiplusStartupInputData, NULL);

					Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);

					if (pBitmap != NULL)
					{
						pBitmap->GetHBITMAP(Gdiplus::Color(), &hResult);

						if (pnHeight != NULL)
							*pnHeight = pBitmap->GetHeight();

						delete pBitmap;
					}

					Gdiplus::GdiplusShutdown(nDiplusToken);

					pStream->Release();
				}

				::GlobalUnlock(hGlobal);
				::GlobalFree(hGlobal);
			}
		}
	}

	return hResult;
}



bool			ExtractOwnIconInfoOfFile( std::wstring& rsIconFilePath, int& rnIconIndex, const std::wstring& rsFile)
{
	size_t nDot = rsFile.rfind(L'.');

	if (nDot == -1)
		return false;
	
	LPCWSTR sExt = &rsFile[nDot];

	if (ExtIsCodeImage(sExt))
	{
		rsIconFilePath		= rsFile;
		rnIconIndex			= 0;

		return true;
	}
	else if (ExtIsIconFile(sExt))
	{
		rsIconFilePath		= rsFile;
		rnIconIndex			= 0;

		return true;
	}
	else if (ExtIsShortcut(sExt))
	{
		return GetShortcutIconLocation(rsIconFilePath, rnIconIndex, rsFile.c_str());
	}
	else if (ExtIsShortcutUrl(sExt))
	{
		std::wstring sFileData;

		if (!ReadTextFromFile(sFileData, rsFile.c_str()))
			return false;
		
		std::transform(sFileData.begin(),sFileData.end(),sFileData.begin(),tolower);

		size_t nStartPos = sFileData.find(L"\niconfile");
		
		if (nStartPos != -1)
		{
			size_t nPos = nStartPos + 9;

			for(WCHAR c = sFileData[nPos]; c == L' ' || c == L'='; ++nPos, c = sFileData[nPos]);

			size_t nEndPos = sFileData.find(L'\r', nPos );

			if (nEndPos != -1)
				rsIconFilePath = sFileData.substr(nPos, nEndPos-nPos);
			else
				rsIconFilePath = sFileData.substr(nPos);

			rnIconIndex = 0;

			nStartPos = sFileData.find(L"\niconindex");

			if (nStartPos != -1)
			{
				nPos = nStartPos + 10;

				for(WCHAR c = sFileData[nPos]; c == L' ' || c == L'='; ++nPos, c = sFileData[nPos]);

				rnIconIndex = _wtoi(&sFileData.c_str()[nPos]);
			}
			
			return true;
		}

		return false;
	}

	return false;
}


static HICON	CreateIconFromGdiPlusBitMap(Gdiplus::Bitmap* pBitmap)
{
	ICONINFO newIcon = {0};

	newIcon.fIcon = TRUE;
	newIcon.hbmMask = CreateBitmap(pBitmap->GetWidth(),pBitmap->GetHeight(),1,1,NULL);

	if (newIcon.hbmMask == NULL)
		return NULL;

	//GetHICON was causing funny alpha issues on some files. The alpha wasn't in the color or mask bitmap from GetIconInfo...
	pBitmap->GetHBITMAP(Gdiplus::Color::Transparent, &newIcon.hbmColor);

	HICON hIcon = NULL;

	if (newIcon.hbmColor != NULL)
	{
		hIcon = CreateIconIndirect(&newIcon);

		DeleteObject(newIcon.hbmColor);
	}

	DeleteObject(newIcon.hbmMask);

	return hIcon;

}



HICON			ExtractIconFromFile(const std::wstring& rsFile, const UINT nIndex)
{
	HICON hIcon = NULL;

	size_t nDot = rsFile.rfind(L'.');

	if (nDot == -1)
		return NULL; //WHAT? WHAT?

	LPCWSTR sExt = rsFile.c_str()+nDot;

	if (ExtIsCodeImage(sExt) || ExtIsIconFile(sExt))
	{
		hIcon = ExtractIcon(g_hInstance, rsFile.c_str(), nIndex);
	}
	else if (ExtIsShortcut(sExt))
	{
		std::wstring sTarget;
		int nIndex;

		if (GetShortcutIconLocation(sTarget, nIndex, rsFile.c_str()))
			hIcon = ExtractIcon(g_hInstance, sTarget.c_str(), nIndex);
		else
			hIcon = ExtractIcon(g_hInstance, rsFile.c_str(), nIndex);
	}
	else
	{
		Gdiplus::GdiplusStartupInput GDiplusStartupInput;
		ULONG_PTR nDiplusToken;

		if (Gdiplus::GdiplusStartup(&nDiplusToken, &GDiplusStartupInput, NULL) != Gdiplus::Ok)
			return NULL;

		Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromFile(rsFile.c_str(), TRUE);

		if (pBitmap != NULL)
		{
			const UINT nHeight = pBitmap->GetHeight();
			const UINT nWidth = pBitmap->GetWidth();

			if (nHeight != 0 && (nWidth%nHeight) == 0)
			{
				const UINT nIconNum = nWidth/nHeight;

				if (nIndex < nIconNum)
				{
					Gdiplus::Bitmap* pSubBitmap = pBitmap->Clone(nIndex*nHeight, 0, nHeight, nHeight, PixelFormat32bppARGB);

					if (pSubBitmap != NULL)
					{
						hIcon = CreateIconFromGdiPlusBitMap(pSubBitmap);

						delete pSubBitmap;
					}
				}
			}

			delete pBitmap;
		}

		Gdiplus::GdiplusShutdown(nDiplusToken);
	}
		
	return hIcon;
}




HICON			GetSpecificIcon( LPCWSTR sFile, const int nIndex)
{
	HICON hIcon = NULL;

	LPCWSTR sDot = wcsrchr(sFile, L'.');

	if (sDot != NULL)
	{
		LPCWSTR sExt = sDot+1;

		if (ExtIsCodeImage(sExt) || ExtIsIconFile(sExt))
		{
			hIcon = ExtractIcon(g_hInstance, sFile, nIndex);
		}
		else if (ExtIsShortcut(sExt))
		{
			std::wstring sTarget;
			int nIndex;

			if (GetShortcutIconLocation(sTarget, nIndex, sFile))
				hIcon = ExtractIcon(g_hInstance, sTarget.c_str(), nIndex);
			else
				hIcon = ExtractIcon(g_hInstance, sFile, nIndex);
		}
		else
		{
			Gdiplus::GdiplusStartupInput GDiplusStartupInput;
			ULONG_PTR nDiplusToken;

			if (Gdiplus::GdiplusStartup(&nDiplusToken, &GDiplusStartupInput, NULL) != Gdiplus::Ok)
				return NULL;

			Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromFile(sFile, TRUE);

			if (pBitmap != NULL)
			{
				hIcon = CreateIconFromGdiPlusBitMap(pBitmap);

				delete pBitmap;
			}

			Gdiplus::GdiplusShutdown(nDiplusToken);
		}
	}

	return hIcon;
}





HIMAGELIST		LoadImageList(const std::wstring& rsFile)
{
	HIMAGELIST hImageList = NULL;

	Gdiplus::GdiplusStartupInput GDiplusStartupInput;
	ULONG_PTR nDiplusToken;

	Gdiplus::GdiplusStartup(&nDiplusToken, &GDiplusStartupInput, NULL);

	Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromFile(rsFile.c_str(), TRUE);

	if (pBitmap != NULL)
	{
		UINT nHeight = pBitmap->GetHeight();
		UINT nWidth = pBitmap->GetWidth();

		UINT nIconNum = nWidth/nHeight;

		hImageList = ImageList_Create(nHeight, nHeight, ILC_COLOR32|ILC_MASK, 0, nIconNum);

		if (nIconNum > 0)
		{
			UINT nPos = 0;

			ICONINFO newIcon = {0};

			newIcon.fIcon = TRUE;
			newIcon.hbmMask = CreateBitmap(nHeight,nHeight,1,1,NULL);

			while(nIconNum--)
			{
				HICON hIcon = NULL;

				Gdiplus::Bitmap* pSubBitmap = pBitmap->Clone(nPos, 0, nHeight, nHeight, PixelFormat32bppARGB);

				nPos += nHeight;

				if (pSubBitmap != NULL)
				{
					//GetHICON was causing funny alpha issues on some files. The alpha wasn't in the color or mask bitmap from GetIconInfo...
					pSubBitmap->GetHBITMAP(Gdiplus::Color::Transparent, &newIcon.hbmColor);

					hIcon = CreateIconIndirect(&newIcon);

					DeleteObject(newIcon.hbmColor);
					newIcon.hbmColor = NULL;

					delete pSubBitmap;
				}						

				const int nIndex = ImageList_GetImageCount(hImageList);

				if ( ImageList_SetImageCount(hImageList, nIndex+1) && hIcon != NULL)
					ImageList_ReplaceIcon(hImageList, nIndex, hIcon);

				if (hIcon != NULL)
					DestroyIcon(hIcon);
			}


			DeleteObject(newIcon.hbmMask);
		}

		delete pBitmap;
	}
	
	Gdiplus::GdiplusShutdown(nDiplusToken);

	return hImageList;
	
}


bool SortIconCB (IconAndInfo& rA, IconAndInfo& rB)
{
	if (rA.bHeight == rB.bHeight)
		return (rA.wBitCount < rB.wBitCount);

	return (GetIconHeight(rA.bHeight) < GetIconHeight(rB.bHeight));
}


void			PreInfoAndInfoList(std::vector<IconAndInfo>& rList)
{
	std::sort(rList.begin(), rList.end(), SortIconCB);

	for(std::vector<IconAndInfo>::const_iterator it = rList.begin();
		it != rList.end();)
	{
		std::vector<IconAndInfo>::const_iterator next = it+1;

		if (next == rList.end())
			return; //We are done

		if (it->bHeight == next->bHeight)
		{
			if (it->wBitCount < next->wBitCount)
				next = rList.erase(it);
		}

		it = next;
	}
}


const IconAndInfo*	FindBestFit(const std::vector<IconAndInfo>& rList, UINT nHeight)
{
	const IconAndInfo* pResult = &rList[0];

	BOOST_FOREACH(const IconAndInfo& rEntry, rList)
	{
		if (IsBetterFit(&rEntry, pResult, nHeight))
			pResult = &rEntry;
	}

	return pResult;
}





bool			BurnIconsTogether(LPCWSTR sDstFile, LPCWSTR sBaseFile, int nBaseIconIndex, LPCWSTR sOverlayFile, int nOverlayIconIndex)
{
	bool bResult = false;

	std::vector<IconAndInfo> baseIcons;
	std::vector<IconAndInfo> overlayIcons;

	if (GenericReadIconDir(sBaseFile, nBaseIconIndex, baseIcons) &&
		GenericReadIconDir(sOverlayFile, nOverlayIconIndex, overlayIcons))
	{
		std::vector<HICON> resultIcons;

		PreInfoAndInfoList(baseIcons);
		PreInfoAndInfoList(overlayIcons);

		int nOverlayIndex = 0;

		BOOST_FOREACH(IconAndInfo& rBase, baseIcons)
		{
			const UINT nBaseHeight = GetIconHeight(rBase.bHeight);

			const IconAndInfo* pOverlay = FindBestFit(overlayIcons, nBaseHeight);
			const UINT nOverlayHeight = GetIconHeight(pOverlay->bHeight);

			POINT basePos = {0,0};
			POINT overlayPos = {nBaseHeight,nBaseHeight};

			overlayPos.x -= nOverlayHeight;
			overlayPos.y -= nOverlayHeight;

			overlayPos.x = max(overlayPos.x, 0);
			overlayPos.y = max(overlayPos.y, 0);

			HICON hIcon = BurnTogether(rBase.hIcon, basePos, pOverlay->hIcon, overlayPos, nBaseHeight);

			if (hIcon != NULL)
				resultIcons.push_back(hIcon);
		}

		if (resultIcons.size())
			bResult = WriteToIconFile(sDstFile, &resultIcons[0], (int)resultIcons.size(), false);

		BOOST_FOREACH(HICON hIcon, resultIcons)
			DestroyIcon(hIcon);
	}


	BOOST_FOREACH(IconAndInfo& rInfo, baseIcons)
		DestroyIcon(rInfo.hIcon);

	BOOST_FOREACH(IconAndInfo& rInfo, overlayIcons)
		DestroyIcon(rInfo.hIcon);

	return bResult;
}



HBITMAP		GetIconAsRGBABitMap(HICON hIcon, DWORD nWidth, DWORD nHeight)
{
	HDC hDC = CreateCompatibleDC(0);

	if (hDC == NULL)
		return NULL;

	HBITMAP hResult = CreateAlphaBitmap(hDC, nWidth, nHeight, NULL);

	HGDIOBJ hSel = SelectObject(hDC, hResult);

	DrawIconEx(hDC, 0, 0, hIcon, nWidth, nHeight, 0, NULL, DI_NORMAL);

	SelectObject(hDC, hSel);

	DeleteDC(hDC);

	return hResult;
}