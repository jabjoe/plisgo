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

#include <ntquery.h>

#include "PlisgoView.h"
#include "PlisgoFSFolderReg.h"
#include "PlisgoFolder.h"
#include "PlisgoData.h"
#include "PlisgoDropSource.h"

#define APSTUDIO_INVOKED
#include "resource.h"

const CLSID	CLSID_PlisgoView		= {0xE8716A04,0x546B,0x4DB1,{0xBB,0x55,0x0A,0x23,0x0E,0x20,0xA0,0xC5}};

const UINT CF_PREFERREDDROPEFFECT = RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);



/////////////////////////////////////////////////////////////////////////////
// CPlisgoView::MenuClickPacket
//


static bool IsEntrySepatator(HMENU hMenu, const UINT nEntry)
{
	MENUITEMINFO info = {0};

	info.cbSize = sizeof(MENUITEMINFO);
	info.fMask = MIIM_FTYPE;

	if (::GetMenuItemInfo(hMenu, nEntry, MF_BYPOSITION, &info))
		return ((info.fType & MFT_SEPARATOR) == MFT_SEPARATOR);

	return false;
}


static int	FindSeperatorPosFromBottom(HMENU hMenu, int nSeperator)
{
	int nPos = 0;

	int nMenuCount = ::GetMenuItemCount(hMenu);

	while(nMenuCount--)
	{
		if (IsEntrySepatator(hMenu, nMenuCount))
		{
			if (nSeperator == 0)
			{
				nPos = nMenuCount;
				break;
			}
			else --nSeperator;
		}
	}
	
	return nPos;
}

static void	MarkSelectedAsCut(HWND hList)
{
	int nItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED) ;

	while(nItem != -1)
	{
		ListView_SetItemState(hList, nItem, LVIS_CUT, LVNI_STATEMASK);
		ListView_RedrawItems(hList, nItem, nItem);
		nItem = ListView_GetNextItem(hList, nItem, LVNI_SELECTED) ;
	}
}


CPlisgoView::MenuClickPacket::MenuClickPacket(PlisgoFSMenu* pThis, PlisgoFSMenuItemClickCB cb)
{
	m_Data.PlisgoMenuPacket.pThis = pThis;
	m_Data.PlisgoMenuPacket.cb = cb;
	m_bIsMenu = true;
}

CPlisgoView::MenuClickPacket::MenuClickPacket(CPlisgoView* pThis, PlisgoViewMenuItemClickCB cb)
{
	m_Data.PlisgoViewPacket.pThis = pThis;
	m_Data.PlisgoViewPacket.cb = cb;
	m_bIsMenu = false;
}

void CPlisgoView::MenuClickPacket::Do()
{
	if (m_bIsMenu)
		(m_Data.PlisgoMenuPacket.pThis->*m_Data.PlisgoMenuPacket.cb)();
	else
		(m_Data.PlisgoViewPacket.pThis->*m_Data.PlisgoViewPacket.cb)();
}





static LPITEMIDLIST	ListViewGetItemIDL(HWND hList, int nItem)
{
	LVITEM item = {0};

	item.mask = LVIF_PARAM;
	item.iItem = nItem;

	if (ListView_GetItem(hList, &item))
		return (LPITEMIDLIST)item.lParam;
	else
		return NULL;
}


static int			ListViewGetItemFromIDL(HWND hList, LPITEMIDLIST pIDL)
{
	const int nItemCount = ListView_GetItemCount(hList);

	for(int n = 0; n < nItemCount; ++n)
	{
		LPITEMIDLIST pIDL2 = ListViewGetItemIDL(hList, n);

		if (ILIsEqual(pIDL, pIDL2))
			return n;
	}

	return -1;
}





CPlisgoView::AsynLoader::AsynLoader(HWND				hWnd,
									IPtrRefIconList		iconList,
									int					nColumnOffset,
									IPtrPlisgoFSRoot	plisgoFSFolder,
									const std::wstring& rsPath)
{
	m_PlisgoFSFolder = plisgoFSFolder;
	m_hWnd			= hWnd;
	m_IconList		= iconList;
	m_hThread		= CreateThread(NULL, 0, ThreadProcCB, this, 0, &m_nThreadID);
	m_bAlive		= true;
	m_nColumnOffset = nColumnOffset;
	m_hEvent		= CreateEvent(NULL, FALSE, FALSE, NULL);
	m_sPath			= rsPath;

	if (m_sPath.length() && (m_sPath.c_str()[m_sPath.length()-1] != L'\\'))
		m_sPath += L"\\";
}


CPlisgoView::AsynLoader::~AsynLoader()
{
	m_bAlive = false;

	DWORD nExitCode;

	while(GetExitCodeThread(m_hThread, &nExitCode))
	{
		SetEvent(m_hEvent);

		if (nExitCode != STILL_ACTIVE)
			break;

		WaitForSingleObject(m_hThread, 200);
	}

	CloseHandle(m_hThread);
	CloseHandle(m_hEvent);
}


void	CPlisgoView::AsynLoader::AddIconJob(int nItem, LPCWSTR sName)
{
	AddTextJob(nItem, -1, sName);
}


void	CPlisgoView::AsynLoader::WakeupWorker()
{
	DWORD nExitCode;

	BOOL bAlive = GetExitCodeThread(m_hThread, &nExitCode);

	if (bAlive)
	{
		if (nExitCode == STILL_ACTIVE)
			SetEvent(m_hEvent);
		else
			bAlive = 0;
	}

	if (!bAlive)
	{
		CloseHandle(m_hThread);
		m_hThread = CreateThread(NULL, 0, ThreadProcCB, this, 0, &m_nThreadID);
	}
}


void	CPlisgoView::AsynLoader::AddTextJob(int nItem, int nSubItem, LPCWSTR sName)
{
	assert(sName != NULL);

	boost::unique_lock<boost::mutex> lock(m_Mutex);

	m_Jobs.push(Job());

	Job& rJob = m_Jobs.back();

	wcscpy_s(rJob.sName, MAX_PATH, sName);

	rJob.nSubItem	= nSubItem;
	rJob.nItem		= nItem;

	WakeupWorker();
}


void	CPlisgoView::AsynLoader::ClearJobs()
{
	boost::unique_lock<boost::mutex> lock(m_Mutex);

	while(m_Jobs.size())
		m_Jobs.pop();
}

#define SIXTEENBYTEALIGH(_a) ((_a%16)?(_a+(16-(_a%16))):_a)


void	CPlisgoView::AsynLoader::DoJob(const Job& rJob)
{
	if (rJob.nSubItem == -1)
	{
		const int nImage = m_PlisgoFSFolder->GetPathIconIndex(m_sPath + rJob.sName, m_IconList);

		if (m_bAlive && nImage != -1)
		{
			LVITEM* pItem = (LVITEM*)calloc(1,sizeof(LVITEM));

			if (pItem != NULL)
			{
				pItem->mask		= LVIF_IMAGE;
				pItem->iImage	= nImage;
				pItem->iItem	= rJob.nItem;

				if (!::PostMessage(m_hWnd, MSG_UPDATELISTITEM, sizeof(LVITEM), (LPARAM)pItem))
					free(pItem);
			}
		}
	}
	else if (m_nColumnOffset != -1)
	{
		std::wstring sText;

		if (m_PlisgoFSFolder->GetPathColumnTextEntry(sText, m_sPath + rJob.sName, rJob.nSubItem-m_nColumnOffset) && m_bAlive)
		{
			const size_t nTextSize		= sizeof(WCHAR)*(sText.length()+1);
			const size_t nAlignedItem	= SIXTEENBYTEALIGH(sizeof(LVITEM));
			const size_t nSize			= nAlignedItem + nTextSize;

			LVITEM* pItem = (LVITEM*)calloc(1,nSize);

			if (pItem != NULL)
			{
				pItem->mask			= LVIF_TEXT;
				pItem->iItem		= rJob.nItem;
				pItem->iSubItem		= rJob.nSubItem;
				pItem->pszText		= (LPWSTR)(((BYTE*)pItem)+nAlignedItem);
				pItem->cchTextMax	= (int)sText.length();

				memcpy_s(pItem->pszText, nTextSize, sText.c_str(), nTextSize);

				if (!::PostMessage(m_hWnd, MSG_UPDATELISTITEM, (WPARAM)nSize, (LPARAM)pItem))
					free(pItem);
			}
		}
	}
}


void	CPlisgoView::AsynLoader::DoWork()
{
	int nEmptyLoop = 0;

	while(m_bAlive)
	{
		Job job;

		bool bFound = false;

		{
			boost::unique_lock<boost::mutex> lock(m_Mutex);

			if (m_Jobs.size() != 0)
			{
				job = m_Jobs.front();
			
				m_Jobs.pop();

				bFound = true;
			}
		}

		if (bFound)
		{
			DoJob(job);
			nEmptyLoop = 0;

			{
				//Check if it's the last, if so, do refresh
				boost::unique_lock<boost::mutex> lock(m_Mutex);

				if (m_Jobs.size() == 0)
					::PostMessage(m_hWnd, MSG_UPDATELISTITEM, 0, 0);
			}
		}
	
		++nEmptyLoop;

		if (nEmptyLoop > 1000)
		{
			if (nEmptyLoop < 1500) //10 seconds of waiting then kill thread
				WaitForMultipleObjects(1, &m_hEvent, TRUE, 1000); //wait for a second
			else
				break; // We are done.
		}
	}
}


DWORD WINAPI CPlisgoView::AsynLoader::ThreadProcCB(LPVOID lpParameter)
{
	static IUnknown* pExplorer = NULL;

	SHGetInstanceExplorer(&pExplorer);

	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	AsynLoader* pAsynLoader = (AsynLoader*)lpParameter;

	pAsynLoader->DoWork();

	CoUninitialize();

	if (pExplorer != NULL)
		pExplorer->Release();

	return 0;
}


/*
	Class to lazy initilize Common Controls
*/
class CommonControlsInitClass
{
public:
	CommonControlsInitClass()
	{
		InitCommonControls();

		INITCOMMONCONTROLSEX InitCtrls;

        InitCtrls.dwICC = ICC_LISTVIEW_CLASSES;
        InitCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);

        BOOL bRet = InitCommonControlsEx(&InitCtrls);
	}
};

/*
	Function to lazy initilize Common Controls
*/
static void CommonControlsInit()
{
	static CommonControlsInitClass init;
}


// CPlisgoView

/////////////////////////////////////////////////////////////////////////////
// CPlisgoView

CPlisgoView::CPlisgoView() : m_nUIState(SVUIA_DEACTIVATE), m_nSortedColumn(0), 
                       m_bForwardSort(true), m_hWndParent(NULL),
                       m_pContainingFolder(NULL), m_hMenu(NULL), m_hList(NULL),
					   m_nShellNotificationID(0), m_pAsynLoader(NULL)

{
	CommonControlsInit();
	m_bViewInit = 0;
}


CPlisgoView::~CPlisgoView()
{
	if (m_pContainingFolder != NULL)
		m_pContainingFolder->Release();

    if ( NULL != m_hMenu )
       DestroyMenu ( m_hMenu );
}


int CALLBACK CPlisgoView::DefaultCompareItems(LPARAM l1, LPARAM l2, LPARAM lData)
{
	LPITEMIDLIST	pA			= (LPITEMIDLIST)l1;
	LPITEMIDLIST	pB			= (LPITEMIDLIST)l2;
	CPlisgoView*		pThis		= (CPlisgoView*)lData;

	DWORD nAttrA = 0, nAttrB = 0;

	if (pThis->m_pContainingFolder->GetAttributesOf(1, const_cast<LPCITEMIDLIST*>(&pA), &nAttrA) == S_OK
		&&
		pThis->m_pContainingFolder->GetAttributesOf(1, const_cast<LPCITEMIDLIST*>(&pB), &nAttrB) == S_OK)
	{
		if ((nAttrA&SFGAO_FOLDER) != (nAttrB&SFGAO_FOLDER))
		{
			if (pThis->m_bForwardSort)
				return (nAttrA&SFGAO_FOLDER)?-1:1;
			else
				return (nAttrA&SFGAO_FOLDER)?1:-1;
		}
	}
	
	if (pThis->m_nSortedColumn < (int)pThis->m_nColumnIDMap.size())
	{
		int nColumnID = pThis->m_nColumnIDMap[pThis->m_nSortedColumn];

		int nResult = (short)HRESULT_CODE(pThis->m_pContainingFolder->CompareIDs(nColumnID, pA, pB));

		nResult *= (pThis->m_bForwardSort)?1:-1;

		return nResult;
	}
	else
	{
		int nPlisgoColumnIndex = pThis->m_nSortedColumn-(int)pThis->m_nColumnIDMap.size();

		WCHAR sAName[MAX_PATH];
		WCHAR sBName[MAX_PATH];

		bool bA = (pThis->m_pContainingFolder->GetItemName(pA, sAName, MAX_PATH) == S_OK);
		bool bB = (pThis->m_pContainingFolder->GetItemName(pB, sBName, MAX_PATH) == S_OK);

		if (bA && bB)
		{
			std::wstring sPath = pThis->m_pContainingFolder->GetPath() + L"\\";

			if (pThis->m_PlisgoFSFolder->GetColumnType(nPlisgoColumnIndex) == PlisgoFSRoot::TEXT)
			{
				std::wstring sAEntry;
				std::wstring sBEntry;

				pThis->m_PlisgoFSFolder->GetPathColumnTextEntry(sAEntry, sPath+sAName ,nPlisgoColumnIndex);
				pThis->m_PlisgoFSFolder->GetPathColumnTextEntry(sBEntry, sPath+sBName ,nPlisgoColumnIndex);

				if (pThis->m_bForwardSort)
					return wcscmp(sAEntry.c_str(), sBEntry.c_str());
				else
					return wcscmp(sBEntry.c_str(), sAEntry.c_str());
			}
			else
			{
				if (pThis->m_PlisgoFSFolder->GetColumnType(nPlisgoColumnIndex) == PlisgoFSRoot::INT)
				{
					int nA = 0, nB = 0;

					pThis->m_PlisgoFSFolder->GetPathColumnIntEntry(nA, sPath+sAName ,nPlisgoColumnIndex);
					pThis->m_PlisgoFSFolder->GetPathColumnIntEntry(nB, sPath+sBName ,nPlisgoColumnIndex);

					if (pThis->m_bForwardSort)
						return (nA > nB);
					else
						return (nA < nB);
				}
				else
				{
					double nA = 0.0, nB = 0.0;

					pThis->m_PlisgoFSFolder->GetPathColumnFloatEntry(nA, sPath+sAName ,nPlisgoColumnIndex);
					pThis->m_PlisgoFSFolder->GetPathColumnFloatEntry(nB, sPath+sBName ,nPlisgoColumnIndex);

					if (pThis->m_bForwardSort)
						return (nA > nB);
					else
						return (nA < nB);
				}
			}
		}
		else if (!bA && !bB)
			return 0;
		else if (bA)
		{
			if (pThis->m_bForwardSort)
				return 1;
			else
				return -1;
		}
		else if (!pThis->m_bForwardSort)
			return 1;
		else
			return -1;
	}
}


bool		 CPlisgoView::Init ( CPlisgoFolder* pContainingFolder )
{
	assert(pContainingFolder != NULL);

	m_PlisgoFSFolder = pContainingFolder->GetPlisgoFSLocal();

	if (m_PlisgoFSFolder.get() == NULL)
		return false;

	m_pContainingFolder = pContainingFolder;
	m_pContainingFolder->AddRef();

	return true;
}


STDMETHODIMP CPlisgoView::GetWindow ( HWND* phWnd )
{
    *phWnd = m_hWnd;
    return S_OK;
}

STDMETHODIMP CPlisgoView::GetCurrentInfo ( LPFOLDERSETTINGS pFS )
{
    *pFS = m_FolderSettings;
    return S_OK;
}


STDMETHODIMP CPlisgoView::CreateViewWindow(	LPSHELLVIEW			pPrevView, 
											LPCFOLDERSETTINGS	pfs,
                                            LPSHELLBROWSER		psb, 
                                            LPRECT				prcView,
											HWND*				phWnd )
{
    *phWnd = NULL;

    m_ShellBrowser = psb;
    m_FolderSettings = *pfs;

	m_ShellBrowser->GetWindow( &m_hWndParent );

    if ( NULL == Create ( m_hWndParent, *prcView, L"PlisgoWindow", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP ) )
       return E_FAIL;

	SetDlgCtrlID(1121); //Ensure it's the same ID as the original.

    *phWnd = m_hWnd;

	if (m_CommDlgBrowser.p != NULL)
		m_CommDlgBrowser.Release();

	m_ShellBrowser->QueryInterface(IID_ICommDlgBrowser, (void**)&m_CommDlgBrowser);

	if (LOBYTE(LOWORD(GetVersion())) > 5/*Vista*/ && m_CommDlgBrowser.p != NULL)
	{
		HWND hPrev = ::GetDlgItem(m_hWndParent, 1120); //Place holder controller in open dialog template

		if (hPrev != NULL && hPrev != INVALID_HANDLE_VALUE)
			::SetWindowPos(m_hWnd, hPrev, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
	}


	SHChangeNotifyEntry entry;

	entry.fRecursive = TRUE;
	entry.pidl = m_pContainingFolder->GetIDList();

	m_nShellNotificationID = SHChangeNotifyRegister(m_hWnd, SHCNRF_ShellLevel | SHCNRF_RecursiveInterrupt,
													SHCNE_ATTRIBUTES | SHCNE_CREATE | SHCNE_DELETE | SHCNE_RENAMEITEM |
													SHCNE_UPDATEITEM | SHCNE_UPDATEDIR | SHCNE_MKDIR | SHCNE_RMDIR |
													SHCNE_RENAMEFOLDER | SHCNE_RMDIR, WM_PLISGOVIEWSHELLMESSAGE, 1, &entry);


    return S_OK;
}


STDMETHODIMP CPlisgoView::DestroyViewWindow()
{
    UIActivate ( SVUIA_DEACTIVATE );

    if ( NULL != m_hMenu )
       DestroyMenu ( m_hMenu );

	if (m_pAsynLoader != NULL)
		delete m_pAsynLoader;

	ListView_DeleteAllItems(m_hList);
	::DestroyWindow(m_hList);

	SHChangeNotifyDeregister(m_nShellNotificationID);

	if (m_hWnd != NULL)
	    DestroyWindow();

    return S_OK;
}


HRESULT		 CPlisgoView::SelectItem(LPCITEMIDLIST pIDLItem, UINT uFlags)
{
	if (m_bViewInit)
	{
		int nItem = ListViewGetItemFromIDL(m_hList, const_cast<LPITEMIDLIST>(pIDLItem));

		if (nItem == -1)
			return E_INVALIDARG;

		if (uFlags & SVSI_SELECT)
		{
			ListView_SetItemState(m_hList, nItem, LVNI_SELECTED, LVNI_SELECTED);
		}
		else if (uFlags & SVSI_DESELECT)
		{
			ListView_SetItemState(m_hList, nItem, 0, LVNI_SELECTED);
		}
		else return E_NOTIMPL;

		return S_OK;
	}
	else
	{
		// Having to support this isn't a nice thing to do!

		if (uFlags & SVSI_SELECT)
		{
			m_InitSelection.push_back(ILClone(pIDLItem));
		}
		else if (uFlags & SVSI_DESELECT)
		{
			for(std::vector<LPITEMIDLIST>::const_iterator it = m_InitSelection.begin();
				it != m_InitSelection.end(); ++it)
			{
				if (ILIsEqual(pIDLItem, *it))
				{
					ILFree(*it);
					m_InitSelection.erase(it);
					break;
				}
			}
		}
		else return E_NOTIMPL;

		return S_OK;
	}
}


HRESULT		 CPlisgoView::GetItemObject(UINT uItem, REFIID rIID, LPVOID* pPv)
{
	HRESULT hr = E_NOINTERFACE;

	LPCITEMIDLIST* pSelection = NULL;
	int nSelected = 0;

	switch(uItem)
	{
		case SVGIO_ALLVIEW:
			GetSelection(pSelection, nSelected, LVNI_ALL);
			break;
		case SVGIO_SELECTION:
		case SVGIO_CHECKED:
			GetSelection(pSelection, nSelected, LVNI_SELECTED);
			break;
	}

	if (nSelected)
		hr = m_pContainingFolder->GetUIObjectOf(m_hWnd, nSelected, pSelection, rIID, 0, pPv);
	else
		hr = m_pContainingFolder->CreateViewObject(m_hWnd, rIID, pPv);

	if (pSelection != NULL)
		delete[] pSelection;

	return hr;
}


void		 CPlisgoView::GetSelection(WStringList& rSelection)
{
	std::wstring sPath = m_pContainingFolder->GetPath();

	sPath += L"\\";

	int nItem = ListView_GetNextItem(m_hList, -1, LVNI_SELECTED);

	while(nItem != -1)
	{
		WCHAR sBuffer[MAX_PATH] = {0};

		LPITEMIDLIST pIDL = ListViewGetItemIDL(m_hList, nItem);

		LRESULT hr = m_pContainingFolder->GetItemName(pIDL, sBuffer, MAX_PATH);

		if (hr == S_OK)
		{
			size_t nIndex = rSelection.size();

			rSelection.push_back(sPath);
			rSelection[nIndex] += sBuffer;
		}

		nItem = ListView_GetNextItem(m_hList, nItem, LVNI_SELECTED);
	}
}


void		 CPlisgoView::GetSelection(LPCITEMIDLIST*& rpSelection, int& rnSelected, LPARAM nItemFlag)
{
	rpSelection = NULL;

	rnSelected= 0;

	{
		int nItem = ListView_GetNextItem(m_hList, -1, nItemFlag);

		while(nItem != -1)
		{
			++rnSelected;

			nItem = ListView_GetNextItem(m_hList, nItem, nItemFlag);
		}
	}


	if (rnSelected > 0)
	{
		rpSelection = new LPCITEMIDLIST[rnSelected];

		LPCITEMIDLIST* pSelectionPos = rpSelection;

		int nItem = ListView_GetNextItem(m_hList, -1,nItemFlag);

		while(nItem != -1 && rnSelected != 0)
		{
			LPITEMIDLIST pIDL = ListViewGetItemIDL(m_hList, nItem);

			if (pIDL != NULL)
				*pSelectionPos++ = pIDL;
			else
				--rnSelected;

			nItem = ListView_GetNextItem(m_hList, nItem, nItemFlag);
		}
	}

	if (rnSelected == 0 && rpSelection != NULL)
	{
		delete[] rpSelection;
		rpSelection = NULL;
	}
}


STDMETHODIMP CPlisgoView::Refresh()
{
	//Preserve scroll position, ok default namespace doesn't do this, but it's useful.
	int nHorzScrollPos = ::GetScrollPos(m_hList, SB_HORZ);
	int nVertScrollPos = ::GetScrollPos(m_hList, SB_VERT);

	if (m_pAsynLoader != NULL)
		m_pAsynLoader->ClearJobs();

	ListView_DeleteAllItems(m_hList);
    FillList();

	if (m_IconList.get() != NULL)
		ListView_Scroll(m_hList, nHorzScrollPos, nVertScrollPos*m_IconList->GetHeight());
		//I don't know why nHorzScrollPos doesn't need putting into pixels, it should acording to msdn....

    return S_OK;
}


STDMETHODIMP CPlisgoView::CreateViewWindow2(LPSV2CVW2_PARAMS lpParams)
{
	HRESULT hr = CreateViewWindow(	lpParams->psvPrev,
									lpParams->pfs,
									lpParams->psbOwner,
									lpParams->prcView,
									&lpParams->hwndView);

	return hr;
}


STDMETHODIMP CPlisgoView::GetView(SHELLVIEWID* pVID, ULONG uView)
{
	if (pVID == NULL)
		return E_POINTER;

	switch(m_FolderSettings.ViewMode)
	{
	case FVM_DETAILS:	*pVID = VID_Details; break;
	case FVM_TILE:		*pVID = VID_Tile; break;
	case FVM_THUMBNAIL:	*pVID = VID_Thumbnails; break;
	default: return E_NOTIMPL;
	}

	return S_OK;
}


STDMETHODIMP CPlisgoView::UIActivate ( UINT nState )
{
    if ( m_nUIState == nState )
        return S_OK;
    
    HandleActivate ( nState );

    return S_OK;
}


void		 CPlisgoView::DoUndo()
{
	//Love to know how..... I think I would have to do it all........
}



HGLOBAL		 CPlisgoView::GetSelectionAsDropData()
{
	WStringList selection;

	GetSelection(selection);

	if (selection.size() == 0)
		return NULL;


	int nDropDataSize = sizeof(DROPFILES);

	for(WStringList::const_iterator it = selection.begin(); it != selection.end(); ++it)
		nDropDataSize += (1+(int)it->length()) * sizeof(WCHAR);

	nDropDataSize += sizeof(WCHAR);

	HGLOBAL hDropData = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, nDropDataSize);

	if (hDropData == NULL)
		return NULL;
	
	LPDROPFILES pDropFiles = (LPDROPFILES)GlobalLock(hDropData);

	pDropFiles->pFiles = sizeof(DROPFILES);
	pDropFiles->fWide = TRUE;

	LPWSTR sDropFiles = (LPWSTR)(((char*)pDropFiles) + sizeof(DROPFILES));
	size_t nRemaining = (nDropDataSize-sizeof(DROPFILES))/sizeof(WCHAR);

	for(WStringList::iterator it = selection.begin(); it != selection.end(); ++it)
	{
		EnsureWin32Path(*it);

		wcscpy_s(sDropFiles, nRemaining, it->c_str());
		sDropFiles[it->length()] = L'\0';

		size_t nSize = 1+it->length();

		sDropFiles += nSize;
		nRemaining -= nSize;
	}

	sDropFiles[0] = L'\0';

	GlobalUnlock(hDropData);

	return hDropData;
}


void		 CPlisgoView::PutSelectedToClipboard(const bool bMove)
{
	HGLOBAL  hEffect = ::GlobalAlloc(GHND, sizeof(DWORD)); 

	if (hEffect == NULL)
		return;

	DWORD* pnDropEffect = (DWORD*)GlobalLock(hEffect);
	*pnDropEffect = (bMove)?DROPEFFECT_MOVE:DROPEFFECT_COPY;
	GlobalUnlock(hEffect);

	HGLOBAL hDropData = GetSelectionAsDropData();

	if (hDropData == NULL)
	{
		GlobalFree(hEffect);
		return;
	}

	if( ::OpenClipboard(NULL) )
	{
		::EmptyClipboard();
		::SetClipboardData(CF_PREFERREDDROPEFFECT, hEffect);
		::SetClipboardData( CF_HDROP, hDropData );
		::CloseClipboard();
	}
	else
	{
		GlobalFree(hEffect);
		GlobalFree(hDropData);
	}
}



STDMETHODIMP CPlisgoView::TranslateAccelerator(LPMSG pMsg)
{
	if (ListView_GetEditControl(m_hList) == NULL)
	{
		if (pMsg->message == WM_KEYDOWN)
		{
			if (pMsg->wParam == VK_F5)
			{
				Refresh();

				return S_OK;
			}
			else if (pMsg->wParam == VK_F2)
			{
				int nItem = ListView_GetNextItem(m_hList, -1, LVNI_SELECTED);

				if (nItem != -1)
				{
					ListView_EditLabel(m_hList, nItem);

					if (m_CommDlgBrowser.p != NULL)
						m_CommDlgBrowser->OnStateChange(this, CDBOSC_RENAME);

					return S_OK;
				}
			}
			else if (pMsg->wParam == VK_DELETE)
			{
				WStringList selection;

				GetSelection(selection);

				if (selection.size() == 0)
					return S_FALSE;

				SHFILEOPSTRUCT shellOp = {0};

				shellOp.hwnd = m_hWnd;
				shellOp.wFunc = FO_DELETE;

				if (!GetAsyncKeyState(VK_LSHIFT) && !GetAsyncKeyState(VK_RSHIFT))
					shellOp.fFlags = FOF_ALLOWUNDO;
				else
					shellOp.fFlags = FOF_WANTNUKEWARNING;
				
				std::wstring sFiles;

				std::vector<bool> isFolder(selection.size());

				for(WStringList::iterator it = selection.begin(); it != selection.end(); ++it)
				{
					EnsureWin32Path(*it);

					if (GetFileAttributes(it->c_str()) & FILE_ATTRIBUTE_DIRECTORY)
					{
						isFolder[it-selection.begin()] = true;
						//Remove anything under the folder
						sFiles += *it + L"\\*";
						sFiles += L'\0';
					}

					sFiles += *it;
					sFiles += L'\0';
				}

				shellOp.pFrom = sFiles.c_str();

				HRESULT hr = S_OK;

				int nError = SHFileOperation(&shellOp);

				if (nError != 0)
					return S_FALSE;

				for(WStringList::iterator it = selection.begin(); it != selection.end(); ++it)
				{
					const DWORD nAttr = GetFileAttributes(it->c_str());

					if (nAttr == INVALID_FILE_ATTRIBUTES)
					{
						if (isFolder[it-selection.begin()])
							SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHW|SHCNF_FLUSH, it->c_str(), NULL);
						else
							SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW|SHCNF_FLUSH, it->c_str(), NULL);
					}
					else hr = S_FALSE; //Still there..... (WHY DID SHFileOperation RETURN 0!?)
				}

				return hr;
			}
			else if (pMsg->wParam == VK_BACK)
			{
				LPITEMIDLIST pParentIDL;

				if (m_pContainingFolder->GetCurFolder(&pParentIDL) == S_OK)
				{
					LPITEMIDLIST pChild = ILFindLastID(pParentIDL);

					pChild->mkid.cb = 0;

					m_ShellBrowser->BrowseObject(pParentIDL, SBSP_DEFBROWSER);

					return S_OK;
				}
			}
			else
			{
				if (GetAsyncKeyState(VK_CONTROL))
				{
					if (GetAsyncKeyState('Z'))
					{
						DoUndo();
						Refresh();
					}
					else if (GetAsyncKeyState('X'))
					{
						PutSelectedToClipboard(true);
						MarkSelectedAsCut(m_hList);
					}
					else if (GetAsyncKeyState('C'))
					{
						PutSelectedToClipboard(false);
					}
					else if (GetAsyncKeyState('V'))
					{
						OnPaste();
						Refresh();
					}
					else if (GetAsyncKeyState('A'))
					{
						ListView_SetItemState(m_hList,-1, LVNI_SELECTED, LVNI_SELECTED);
					}

					return S_OK;
				}
			}
		}
	}

	TranslateMessage(pMsg);
	DispatchMessage(pMsg);

	return S_OK;
}


STDMETHODIMP CPlisgoView::QueryStatus(const GUID* pguidCmdGroup, ULONG cCmds,
									OLECMD prgCmds[], OLECMDTEXT* pCmdText )
{
    if ( NULL == prgCmds )
        return E_POINTER;

	// TODO: INVESTIGAGE

    // The only useful standard command I've figured out is "refresh".  I've put
    // some trace messages in so you can see the other commands that the
    // browser sends our way.  If you can figure out what they're all for,
    // let me know!

    if ( NULL == pguidCmdGroup )
    {
        for ( UINT u = 0; u < cCmds; u++ )
		{
            ATLTRACE(">> Query - DEFAULT: %u\n", prgCmds[u]);

            switch ( prgCmds[u].cmdID )
            {
            case OLECMDID_REFRESH:
                prgCmds[u].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
            break;
            }
        }

        return S_OK;
    }
    else if ( CGID_Explorer == *pguidCmdGroup )
    {
        for ( UINT u = 0; u < cCmds; u++ )
        {
            ATLTRACE(">> Query - EXPLORER: %u\n", prgCmds[u]);
        }
    }
    else if ( CGID_ShellDocView == *pguidCmdGroup )
    {
        for ( UINT u = 0; u < cCmds; u++ )
        {
            ATLTRACE(">> Query - DOCVIEW: %u\n", prgCmds[u]);
        }
    }
	else
	{
		ATLTRACE(">> Exec - UNKNOWN: %u\n", 0);
	}

    return OLECMDERR_E_UNKNOWNGROUP;
}


STDMETHODIMP CPlisgoView::Exec(	const GUID* pguidCmdGroup, DWORD nCmdID,
                                DWORD nCmdExecOpt, VARIANTARG* pvaIn,
                                VARIANTARG* pvaOut )
{
	HRESULT hrRet = OLECMDERR_E_UNKNOWNGROUP;

    // The only standard command we act on is "refresh".  I've put
    // some trace messages in so you can see the other commands that the
    // browser sends our way.  If you can figure out what they're all for,
    // let me know!


    if ( NULL == pguidCmdGroup )
    {
		ATLTRACE(">> Exec - DEFAULT: %u\n", nCmdID);

		if ( OLECMDID_REFRESH == nCmdID )
        {
			Refresh();
			hrRet = S_OK;
        }
    }
    else if ( CGID_Explorer == *pguidCmdGroup)
    {
        ATLTRACE(">> Exec - EXPLORER : %u\n", nCmdID);
    }
    else if ( CGID_ShellDocView == *pguidCmdGroup )
    {
        ATLTRACE(">> Exec - DOCVIEW: %u\n", nCmdID);
    }
	else
	{
		ATLTRACE(">> Exec - UNKNOWN: %u\n", nCmdID);
	}

    return hrRet;
}


LRESULT		 CPlisgoView::OnCreate(	UINT uMsg, WPARAM wParam, 
									LPARAM lParam, BOOL& rbHandled )
{
	DWORD dwListStyles			=	WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CHILDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
									LVS_EDITLABELS | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_AUTOARRANGE;
	DWORD dwListExStyles		=	WS_EX_CLIENTEDGE;

	m_bViewInit = 0;

	m_hList = ::CreateWindowEx (	dwListExStyles, WC_LISTVIEW, L"FolderView", dwListStyles,
										0, 0, 0, 0, m_hWnd, (HMENU) 101, 
										g_hInstance, 0 );
	
    if ( NULL == m_hList )
        return HRESULTTOLRESULT(HRESULT_FROM_WIN32(GetLastError()));

	::SetWindowLong(m_hList, GWL_ID, 1);

	ListView_SetExtendedListViewStyle(m_hList, LVS_EX_HEADERDRAGDROP );

	{
		CComPtr<IDropTarget> pDropTarget = NULL;

		QueryInterface(IID_IDropTarget, (void**)&pDropTarget);

		if (pDropTarget != NULL)
		{
			HRESULT nTest = RevokeDragDrop(m_hList);

			nTest = RegisterDragDrop(m_hList, pDropTarget);
		}
	}


    // Attach m_List to the list control and initialize styles, image
    // lists, etc.


	m_nColumnIDMap.clear();


	WCHAR   szTemp[MAX_PATH];


	for(UINT n = 0; n < 7 ; ++n)
	{
		SHELLDETAILS  sd;

		HRESULT hr = m_pContainingFolder->GetDetailsOf(NULL, n, &sd);
		
		LVCOLUMNW       lvColumn = {0};

		lvColumn.mask = LVCF_TEXT;

		if (SUCCEEDED(hr) && hr != S_FALSE)
		{
			lvColumn.mask |= LVCF_FMT;
			lvColumn.fmt = sd.fmt;
			StrRetToBuf(&sd.str, NULL, szTemp, MAX_PATH);

			if (sd.str.uType == STRRET_WSTR)
				CoTaskMemFree(sd.str.pOleStr);
		}
		else
		{
			szTemp[0] = L'\0';
			lvColumn.cx = 0;
		}

		lvColumn.pszText = szTemp;

		lvColumn.mask |= LVCF_WIDTH;

		switch(n)
		{
		case 0: lvColumn.cx = 180; break; //Name
		case 1: lvColumn.cx = 60; break; //Size
		case 2: lvColumn.cx = 120; break; //Type
		case 3: lvColumn.cx = 100; break; //Time
		case 4: lvColumn.cx = 100; break; //Time
		case 5: lvColumn.cx = 100; break; //Time
		case 6: lvColumn.cx = 60; break; //Attrib
		default: lvColumn.cx = 0;
		}

		if (lvColumn.cx == 0)
			break;

		if (m_PlisgoFSFolder->IsStandardColumnDisabled(n))
			break;

		m_nColumnIDMap.push_back(n);

		ListView_InsertColumn(m_hList, n, &lvColumn);
	}

	assert(m_PlisgoFSFolder.get() != NULL);

	for(ULONG n = 0; n < m_PlisgoFSFolder->GetColumnNum(); ++n)
	{
		const std::wstring&					rsText			= m_PlisgoFSFolder->GetColumnHeader(n);
		const PlisgoFSRoot::ColumnAlignment	eAlignment		= m_PlisgoFSFolder->GetColumnAlignment(n);
		const int							nDefaultWidth	= m_PlisgoFSFolder->GetColumnDefaultWidth(n);

		LVCOLUMNW   lvColumn = {0};

		if (eAlignment == PlisgoFSRoot::RIGHT)
			lvColumn.fmt |= LVCFMT_RIGHT;
		else if (eAlignment == PlisgoFSRoot::LEFT)
			lvColumn.fmt |= LVCFMT_LEFT;
		else
			lvColumn.fmt |= LVCFMT_CENTER;

		lvColumn.mask |= LVCF_WIDTH;

		if (rsText.length() > 0)
		{
			lvColumn.mask		|= LVCF_TEXT|LVCF_FMT;
			lvColumn.pszText	= (LPWSTR)rsText.c_str();
		}
		else
		{
			lvColumn.mask		|= LVCF_TEXT;
			lvColumn.pszText	= L"";
		}

		const int nCharWidth = ListView_GetStringWidth(m_hList, "~");

		if (nDefaultWidth == -1)
		{
			const int nWidth = ListView_GetStringWidth(m_hList, lvColumn.pszText);

			lvColumn.cx = nWidth + nCharWidth*2;
		}
		else lvColumn.cx = nCharWidth*nDefaultWidth;
	
		ListView_InsertColumn(m_hList, m_nColumnIDMap.size()+n, &lvColumn);
	}

	LV_COLUMN lvColumn = {0};

	lvColumn.mask = LVCF_FMT;

    return 0;
}


LRESULT		 CPlisgoView::OnSize(	UINT uMsg, WPARAM wParam, 
									LPARAM lParam, BOOL& rbHandled )
{
	if (::IsWindow(m_hList))
		::MoveWindow( m_hList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE );

    return 0;
}


LRESULT		 CPlisgoView::OnSetFocus(	UINT uMsg, WPARAM wParam,
										LPARAM lParam, BOOL& rbHandled )
{
	::SetFocus(m_hList);

	if (m_CommDlgBrowser.p != NULL)
		m_CommDlgBrowser->OnStateChange(this, CDBOSC_SETFOCUS);

    return 0;
}

LRESULT		 CPlisgoView::OnKillFocus(	UINT uMsg, WPARAM wParam,
										LPARAM lParam, BOOL& rbHandled )
{
	if (m_CommDlgBrowser.p != NULL)
		m_CommDlgBrowser->OnStateChange(this, CDBOSC_KILLFOCUS);

    return 0;
}



void		 CPlisgoView::AddItem(LPITEMIDLIST pIDL)
{
    LVITEM lvi = {0};

    lvi.mask	= LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    lvi.iItem	= ListView_GetItemCount(m_hList);
    lvi.iImage	= I_IMAGECALLBACK;
	lvi.lParam	= (LPARAM)ILClone(pIDL); //THIS MUST BE FREED
	lvi.pszText	= LPSTR_TEXTCALLBACKW;

	ListView_InsertItem(m_hList, &lvi);

	m_IconSources.resize(m_IconSources.size()+1); //Grow by one
}


bool		 CPlisgoView::DeleteItem(LPITEMIDLIST pIDL)
{
	int nItem = ListViewGetItemFromIDL(m_hList, pIDL);

	if (nItem != -1)
	{
		ListView_DeleteItem(m_hList, nItem);

		m_IconSources.erase(m_IconSources.begin()+nItem);

		return true;
	}

	return false;
}



void		 CPlisgoView::FillList()
{
	CComPtr<IEnumIDList> pEnum;
	LPITEMIDLIST pidl = NULL;

    HRESULT hr = m_pContainingFolder->EnumObjects ( m_hWnd, SHCONTF_NONFOLDERS|SHCONTF_FOLDERS, &pEnum );
    
    if ( FAILED(hr) )
        return;

	::SendMessage(m_hList, WM_SETREDRAW, (WPARAM)FALSE, 0);

	DWORD nFetched;

	m_IconSources.clear();

	std::list<LPITEMIDLIST> pids;

	DWORD nFlags = 0;

#if (NTDDI_VERSION < NTDDI_LONGHORN)
#define CDB2GVF_NOINCLUDEITEM       0x00000010
#endif

	const bool bFilter = m_CommDlgBrowser.p != NULL && m_CommDlgBrowser->GetViewFlags(&nFlags) == S_OK &&
							nFlags != CDB2GVF_NOINCLUDEITEM && nFlags != CDB2GVF_SHOWALLFILES;


	while ( pEnum->Next(1, &pidl, &nFetched) == S_OK )
	{
		ATLASSERT(1 == nFetched);

		if (!bFilter || m_CommDlgBrowser->IncludeObject(this, pidl) == S_OK)
			pids.push_back(ILClone(ILFindLastID(pidl)));
	}

	m_IconSources.resize(pids.size());
	ListView_SetItemCount(m_hList, pids.size());
	LVITEM lvi = {0};

	lvi.mask	= LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iImage	= I_IMAGECALLBACK;
	lvi.pszText	= LPSTR_TEXTCALLBACKW;
	lvi.iItem	= 0;

	for(std::list<LPITEMIDLIST>::const_iterator it = pids.begin(); it != pids.end(); ++lvi.iItem, ++it )
	{
		lvi.lParam	= (LPARAM)*it; //THIS MUST BE FREED

		ListView_InsertItem(m_hList, &lvi);
	}

	m_nSortedColumn = 0;
	m_bForwardSort = true;

	ListView_SortItems(m_hList, DefaultCompareItems, (LPARAM)this);

	::SendMessage(m_hList, WM_SETREDRAW, (WPARAM)TRUE, 0);
	::InvalidateRect(m_hList, NULL, TRUE);
	::UpdateWindow(m_hList);

	if (m_InitSelection.size())
	{
		for(std::vector<LPITEMIDLIST>::const_iterator it = m_InitSelection.begin();
			it != m_InitSelection.end(); ++it)
		{
			SelectItem(*it, SVSI_SELECT);
			ILFree(*it);
		}

		m_InitSelection.clear();
	}
}


void		 CPlisgoView::HandleActivate ( UINT nState )
{
    HandleDeactivate();

	if ( SVUIA_DEACTIVATE != nState )
	{
		ATLASSERT(NULL == m_hMenu);

		m_hMenu = CreateMenu();

		if ( NULL != m_hMenu )
		{
			// Let the browser insert its standard items first.
			OLEMENUGROUPWIDTHS omw = {0};

			m_ShellBrowser->InsertMenusSB ( m_hMenu, &omw );

			//HMENU hEditMenu = GetSubMenu(m_hMenu, FCIDM_MENU_EDIT);


			//InsertMenuItem(hEditMenu, 0, TRUE, 

/*
			if ( SVUIA_ACTIVATE_FOCUS == nState )
				DeleteMenu ( m_hMenu, FCIDM_MENU_EDIT, MF_BYCOMMAND );
*/
			m_ShellBrowser->SetMenuSB ( m_hMenu, NULL, m_hWnd );
		}

		m_ShellBrowser->OnViewWindowActive(this);
	}

    // Save the current state
    m_nUIState = nState;
}


void		 CPlisgoView::HandleDeactivate()
{
    if ( SVUIA_DEACTIVATE != m_nUIState )
    {
		if ( NULL != m_hMenu )
		{
			m_ShellBrowser->SetMenuSB ( NULL, NULL, NULL );
			m_ShellBrowser->RemoveMenusSB ( m_hMenu );
	        
			DestroyMenu ( m_hMenu );    // also destroys the SimpleNSExt submenu
			m_hMenu = NULL;
		}

		m_nUIState = SVUIA_DEACTIVATE;
	}
}


LRESULT		 CPlisgoView::OnCustomViewShellMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& rbHandled)
{
	LONG nEvent = (LONG)lParam;

	PIDLIST_ABSOLUTE pFullIDL = ((PIDLIST_ABSOLUTE*)wParam)[0]; //Second is a terminator

	PUIDLIST_RELATIVE pChildIDL = ILFindChild(m_pContainingFolder->GetIDList(), pFullIDL);

	if (nEvent == SHCNE_UPDATEDIR)
		nEvent = nEvent;

	if (pChildIDL != NULL)
	{
		LPITEMIDLIST pNextIDL = ILGetNext(pChildIDL);

		if (pNextIDL == NULL)
		{
			assert(pChildIDL->mkid.cb == 0);

			Refresh();
		}
		else if (pNextIDL->mkid.cb == 0)
		{

			switch(nEvent)
			{
			case SHCNE_RENAMEITEM:
			case SHCNE_RENAMEFOLDER:
				{
					PIDLIST_ABSOLUTE	pNewFullIDL = ((PIDLIST_ABSOLUTE*)wParam)[1];
					PUIDLIST_RELATIVE	pNewChildIDL = ILFindChild(m_pContainingFolder->GetIDList(), pNewFullIDL);

					pChildIDL = pNewChildIDL; //IDL already changed.
					break;
				}

			case SHCNE_CREATE:
			case SHCNE_MKDIR:
				{
					/*
						Ok, this is a bug work round.
						The IDL of pChildIDL contains file info that is wrong.
						So we recreate the IDL which corrects this.
					*/

					WCHAR sName[MAX_PATH];

					if (m_pContainingFolder->GetItemName(pChildIDL, sName, MAX_PATH) == S_OK)
					{
						PUIDLIST_RELATIVE pNewChildIDL = NULL;

						if (m_pContainingFolder->ParseDisplayName(NULL, NULL, sName, NULL, &pNewChildIDL, NULL) == S_OK)
							pChildIDL = pNewChildIDL;
					}

					if (ListViewGetItemFromIDL(m_hList, pChildIDL) == -1)
						AddItem(pChildIDL);

					return 0;
				}
			case SHCNE_DELETE:
			case SHCNE_RMDIR:
				{
					DeleteItem(pChildIDL);			

					return 0;
				}
			}

			//Refresh child .......

			const int nItem = ListViewGetItemFromIDL(m_hList, pChildIDL);

			if (nItem != -1)
			{
				LVITEM item = {0};

				item.mask		= LVIF_TEXT|LVIF_IMAGE;
				item.pszText	= LPSTR_TEXTCALLBACKW;
				item.iImage		= I_IMAGECALLBACK;
				item.iItem		= nItem;

				ListView_SetItem(m_hList, &item);

				HWND hHeader = ListView_GetHeader(m_hList);

				if (hHeader != NULL && hHeader != INVALID_HANDLE_VALUE)
				{
					const int nColumnCount = Header_GetItemCount(hHeader);

					for(int i = 1; i < nColumnCount; ++i)
					{
						item.mask		= LVIF_TEXT;
						item.iSubItem	= i;

						ListView_SetItem(m_hList, &item);
					}
				}
			}
			else Refresh();
		}
	}
	
    return 0;
}


LRESULT		 CPlisgoView::OnInitMenuPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& rbHandled)
{
    if ( !IsWindow() /*!m_List.IsWindow() ||*/ )
        return DefWindowProc();

	

	bool bEnable = ( ListView_GetSelectedCount(m_hList) > 0 );
	UINT uState = bEnable ? MF_BYCOMMAND : MF_BYCOMMAND | MF_GRAYED;

    return 0;
}


LRESULT		 CPlisgoView::OnListSetfocus ( int nCtrl, LPNMHDR pNmh, BOOL& bHandled )
{
	m_ShellBrowser->OnViewWindowActive ( this );

	HandleActivate ( SVUIA_ACTIVATE_FOCUS );

	if (m_CommDlgBrowser.p != NULL)
		m_CommDlgBrowser->OnStateChange(this, CDBOSC_SETFOCUS);

	return 0;
}


LRESULT		 CPlisgoView::OnListKillfocus ( int nCtrl, LPNMHDR pNmh, BOOL& bHandled )
{
	HandleActivate ( SVUIA_ACTIVATE_NOFOCUS );

	if (m_CommDlgBrowser.p != NULL)
		m_CommDlgBrowser->OnStateChange(this, CDBOSC_KILLFOCUS);

	return 0;
}


LRESULT		 CPlisgoView::OnHeaderItemclick ( int nCtrl, LPNMHDR _pNMH, BOOL& rbHandled )
{
	NMHEADER* pNMH = (NMHEADER*) _pNMH;

	int nClickedItem = pNMH->iItem;

    if ( nClickedItem == m_nSortedColumn )
        m_bForwardSort = !m_bForwardSort;
    else
        m_bForwardSort = true;

    m_nSortedColumn = nClickedItem;

	ListView_SortItems(m_hList, DefaultCompareItems, (LPARAM)this);

    return 0;
}


LRESULT		 CPlisgoView::OnListDeleteitem ( int nCtrl, LPNMHDR pNMH, BOOL& rbHandled )
{
	NMLISTVIEW* pNMLV = (NMLISTVIEW*) pNMH;

	LPITEMIDLIST pIDL = ListViewGetItemIDL(m_hList, pNMLV->iItem);

	ILFree(pIDL);

    return 0;
}


LRESULT		 CPlisgoView::OnItemChanged ( int nCtrl, LPNMHDR pNMH, BOOL& rbHandled )
{
	if (m_CommDlgBrowser.p != NULL)
		m_CommDlgBrowser->OnStateChange(this, CDBOSC_SELCHANGE);

	return 0;
}


LRESULT		 CPlisgoView::OnItemActivated ( int nCtrl, LPNMHDR pNMH, BOOL& rbHandled )
{
	NMITEMACTIVATE* pNMLA = (NMITEMACTIVATE*)pNMH;

	LPITEMIDLIST pIDL = ListViewGetItemIDL(m_hList, pNMLA->iItem);

	if (pIDL == NULL)
		return E_INVALIDARG;

	DWORD nAttr = 0;

	HRESULT hr = m_pContainingFolder->GetAttributesOf(1, (LPCITEMIDLIST*)&pIDL, &nAttr);

	if (FAILED(hr))
		return HRESULTTOLRESULT(hr);

	if (m_CommDlgBrowser.p != NULL)
		m_CommDlgBrowser->OnStateChange(this, CDBOSC_SELCHANGE);

	if (nAttr&(SFGAO_FOLDER|SFGAO_HASSUBFOLDER))
	{
		LPITEMIDLIST pFullIDL = ILCombine(m_pContainingFolder->GetIDList(), pIDL);

		if (pFullIDL != NULL)
		{
			hr = m_ShellBrowser->BrowseObject(pFullIDL, SBSP_SAMEBROWSER);

			ILFree(pFullIDL);
		}
	}
	else
	{
		if (m_CommDlgBrowser.p != NULL) //If it's a dialog view, clicking on a file/folder is signal to close
		{
			m_CommDlgBrowser->OnDefaultCommand(this);
		}
		else
		{
			WCHAR sName[MAX_PATH];

			hr = m_pContainingFolder->GetItemName(pIDL, sName, MAX_PATH);

			if (SUCCEEDED(hr))
			{
				std::wstring sPath = m_pContainingFolder->GetPath() + L"\\" + sName;

				ShellExecuteW(m_hWnd, L"open", sPath.c_str(), NULL, m_pContainingFolder->GetPath().c_str(), SW_SHOWNORMAL);
			}
		}
	}


	return 0;
}


LRESULT		 CPlisgoView::OnDefaultCommand(int nCtrl, LPNMHDR pNmh, BOOL& rbHandled)
{
	if (m_CommDlgBrowser.p != NULL)
		m_CommDlgBrowser->OnDefaultCommand(this);

	return 0;
}


void		 CPlisgoView::RefreshItemImages()
{
	if (InterlockedIncrement(&m_bViewInit) == 1)
	{
		MenuClickPacket::PlisgoViewMenuItemClickCB ViewTypeCD = &CPlisgoView::OnDetailsViewMenuItemClick;

		switch ( m_FolderSettings.ViewMode )
		{
			case FVM_ICON:
			case FVM_TILE:		ViewTypeCD = &CPlisgoView::OnLargeViewMenuItemClick; break;

			case FVM_THUMBSTRIP:
			case FVM_THUMBNAIL:	ViewTypeCD = &CPlisgoView::OnThumbnailsViewMenuItemClick; break;
		}

		if (ViewTypeCD != NULL)
			(this->*ViewTypeCD)();
	}
	else
	{
		const int nItemNum = ListView_GetItemCount(m_hList);

		for(int n = 0; n < nItemNum; ++n)
		{
			LVITEM item = {0};

			item.mask = LVIF_IMAGE;

			if (ListView_GetItem(m_hList, &item))
			{
				if (item.iImage != I_IMAGECALLBACK)
				{
					IconLocation Location;

					if (m_IconList->GetIconLocation(Location, item.iImage))
					{
						const IconLocation& rLoaded = m_IconSources[item.iItem];

						if (rLoaded.nIndex != Location.nIndex ||
							rLoaded.sPath != Location.sPath )
						{
							item.iImage = I_IMAGECALLBACK;
							ListView_SetItem(m_hList, &item);
						}
					}
					else
					{
						item.iImage = I_IMAGECALLBACK;
						ListView_SetItem(m_hList, &item);
					}
				}
			}
		}

		if (m_IconList.get() != NULL)
			m_IconList->RemoveOlderThan(NTMINUTE*2);
	}
}


BOOL		 CPlisgoView::MessageProcessHock(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT& rResult)
{
	switch (nMsg)
	{
	case WM_MENUCHAR:	// only supported by IContextMenu3
		if (m_IMenu3.p != NULL)
		{
			LRESULT lResult = 0;
			m_IMenu3->HandleMenuMsg2 (nMsg, wParam, lParam, &lResult);
			rResult = lResult;
			return TRUE;
		}
		break;

	case WM_DRAWITEM:
	case WM_MEASUREITEM:
		if (wParam) 
			break; // if wParam != 0 then the message is not menu-related

	case WM_INITMENUPOPUP:
		if (m_IMenu2.p != NULL)
		{
			m_IMenu2->HandleMenuMsg (nMsg, wParam, lParam);
			// inform caller that we handled WM_INITPOPUPMENU by ourself
			rResult = (nMsg == WM_INITMENUPOPUP ? 0 : TRUE);
			return TRUE;
		}
		else	if (m_IMenu3.p != NULL)
		{
			m_IMenu3->HandleMenuMsg (nMsg, wParam, lParam);
			// inform caller that we handled WM_INITPOPUPMENU by ourself
			rResult = (nMsg == WM_INITMENUPOPUP ? 0 : TRUE);
			return TRUE;
		}
	case WM_NOTIFY:
		{
			if (::GetDlgCtrlID(m_hList) == wParam)
			{
				LPNMHDR pNotifyHeader = (LPNMHDR)lParam;

				if (pNotifyHeader != NULL &&
					pNotifyHeader->hwndFrom == m_hList &&
					pNotifyHeader->code == NM_CUSTOMDRAW)
				{
					NMCUSTOMDRAW* pCustomDraw = (NMCUSTOMDRAW*)pNotifyHeader;

					if (pCustomDraw->dwDrawStage == CDDS_PREPAINT)
						RefreshItemImages();
				}
			}
		}

	default:
		break;
	}

	return FALSE;
}





void		 CPlisgoView::DoDrop(HDROP hDrop, DWORD nDropEffect)
{
	UINT nFiles = DragQueryFile(hDrop, (UINT)-1, NULL, 0);
		
	if (nFiles > 0)
	{
		std::wstring sSrc;
		std::wstring sDst = m_pContainingFolder->GetPath() + L"\\";
		
		EnsureWin32Path(sDst);

		SHFILEOPSTRUCT shellOp = {0};

		shellOp.hwnd = m_hWnd;
		shellOp.wFunc = GetAsyncKeyState(VK_SHIFT)?FO_MOVE:FO_COPY;
		shellOp.fFlags = FOF_ALLOWUNDO;

		if (nDropEffect & DROPEFFECT_MOVE)
			shellOp.wFunc = FO_MOVE;
		else if (nDropEffect & DROPEFFECT_COPY)
			shellOp.wFunc = FO_COPY;

		for(UINT n = 0; n < nFiles; ++n)
		{
			const UINT		nLen = DragQueryFile(hDrop, n, NULL, 0)+1;
			std::wstring	sSrcFile(nLen, L' ');

			DragQueryFile(hDrop, n, (WCHAR*)sSrcFile.c_str(), nLen);

			if (sSrcFile.find(sDst) != -1)
				shellOp.fFlags |= FOF_RENAMEONCOLLISION;

			sSrc += sSrcFile;
		}

		sSrc += L'\0';
		sDst += L'\0';

		shellOp.pFrom = sSrc.c_str();
		shellOp.pTo = sDst.c_str();

		SHFileOperation(&shellOp);
	}

	Refresh();
}


void		 CPlisgoView::OnPaste()
{
	::OpenClipboard(m_hWnd);

	HDROP hDrop = (HDROP)GetClipboardData(CF_HDROP);

	if (NULL != hDrop)
	{
		DWORD nDropEffect = FO_COPY;

		HANDLE hDropEffect = ::GetClipboardData(CF_PREFERREDDROPEFFECT);

		if (hDropEffect != NULL)
		{
			DWORD* pnDropEffect = (DWORD*)::GlobalLock(hDropEffect);

			if (pnDropEffect != NULL)
				nDropEffect = *pnDropEffect;
			
			::GlobalUnlock(hDropEffect);
		}

		DoDrop(hDrop, nDropEffect);

		::GlobalUnlock(hDrop);
	}

	::CloseClipboard();
}


void		 CPlisgoView::RefreshIconList(LONG nHeight)
{
	IconRegistry* pIconRegistry = PlisgoFSFolderReg::GetSingleton()->GetIconRegistry();

	m_IconList = pIconRegistry->GetRefIconList(nHeight);

	if (m_pAsynLoader != NULL)
		delete m_pAsynLoader;

	m_pAsynLoader = new AsynLoader(m_hWnd, m_IconList, (int)m_nColumnIDMap.size(), m_PlisgoFSFolder, m_pContainingFolder->GetPath());

	ListView_SetImageList(m_hList, m_IconList->GetImageList(), LVSIL_SMALL );
	ListView_SetImageList(m_hList, m_IconList->GetImageList(), LVSIL_NORMAL );

	Refresh();
}


void		 CPlisgoView::OnThumbnailsViewMenuItemClick()
{
	m_FolderSettings.ViewMode = FVM_THUMBNAIL;

	UINT nTmpstyle = ::GetWindowLong(m_hList, GWL_STYLE);

	::SetWindowLong(m_hList, GWL_STYLE, (nTmpstyle & ~LVS_TYPEMASK) | LVS_ICON);

	RefreshIconList(96);
}

void		 CPlisgoView::OnLargeViewMenuItemClick()
{
	m_FolderSettings.ViewMode = FVM_TILE;

	UINT nTmpstyle = ::GetWindowLong(m_hList, GWL_STYLE);

	::SetWindowLong(m_hList, GWL_STYLE, (nTmpstyle & ~LVS_TYPEMASK) | LVS_ICON);

	RefreshIconList(48);
}

void		 CPlisgoView::OnDetailsViewMenuItemClick()
{
	m_FolderSettings.ViewMode = FVM_DETAILS;

	UINT nTmpstyle = ::GetWindowLong(m_hList, GWL_STYLE);

	::SetWindowLong(m_hList, GWL_STYLE, (nTmpstyle & ~LVS_TYPEMASK) | LVS_REPORT);
	
	RefreshIconList(16);
}



void		 CPlisgoView::InsertViewToMenu(HMENU hMenu, std::vector<MenuClickPacket>& rClickEvents, const UINT nIDOffset, int nPos)
{
	UINT nCheck = MF_CHECKED | MF_USECHECKBITMAPS;

	MENUITEMINFO itemInfo = {0};

	itemInfo.cbSize		= sizeof(MENUITEMINFO);
	itemInfo.fMask		= MIIM_STRING|MIIM_ID|MIIM_STATE;
	itemInfo.fType		= MF_STRING;
	itemInfo.fState		= (m_FolderSettings.ViewMode == FVM_THUMBNAIL)?MFS_CHECKED:MFS_ENABLED;
	itemInfo.dwTypeData	= L"Thumbnails";
	itemInfo.cch		= 11;
	itemInfo.wID		= nIDOffset+(UINT)rClickEvents.size();
	rClickEvents.push_back(MenuClickPacket(this,&CPlisgoView::OnThumbnailsViewMenuItemClick));

	::InsertMenuItem(hMenu, nPos, TRUE, &itemInfo);

	itemInfo.fState		= (m_FolderSettings.ViewMode == FVM_TILE)?MFS_CHECKED:MFS_ENABLED;
	itemInfo.dwTypeData	= L"Large";
	itemInfo.cch		= 6;
	itemInfo.wID		= nIDOffset+(UINT)rClickEvents.size();
	rClickEvents.push_back(MenuClickPacket(this,&CPlisgoView::OnLargeViewMenuItemClick));

	::InsertMenuItem(hMenu, nPos+1, TRUE, &itemInfo);

	itemInfo.fState		= (m_FolderSettings.ViewMode == FVM_DETAILS)?MFS_CHECKED:MFS_ENABLED;
	itemInfo.dwTypeData	= L"Details";
	itemInfo.cch		= 8;
	itemInfo.wID		= nIDOffset+(UINT)rClickEvents.size();
	rClickEvents.push_back(MenuClickPacket(this,&CPlisgoView::OnDetailsViewMenuItemClick));

	::InsertMenuItem(hMenu, nPos+2, TRUE, &itemInfo);
}


void		 CPlisgoView::InsertPasteToMenu(HMENU hMenu, std::vector<MenuClickPacket>& rClickEvents, const UINT nIDOffset, int nPos)
{
	BOOL bCanPaste = FALSE;

	if (::OpenClipboard(NULL))
	{
		HDROP hDrop = (HDROP)::GetClipboardData(CF_HDROP);

		if (NULL != hDrop)
		{
			UINT nFiles = DragQueryFile(hDrop, (UINT)-1, NULL, 0);
			
			if (nFiles > 0)
				bCanPaste = TRUE;

			GlobalUnlock(hDrop);

		}

		::CloseClipboard();
	}

	MENUITEMINFO itemInfo = {0};

	itemInfo.cbSize		= sizeof(MENUITEMINFO);
	itemInfo.fMask		= MIIM_TYPE;
	itemInfo.fType		= MF_SEPARATOR;

	::InsertMenuItem(hMenu, nPos++, TRUE, &itemInfo);

	itemInfo.fMask		= MIIM_STRING|((bCanPaste)?0:MIIM_STATE)|MIIM_ID;
	itemInfo.fType		= MF_STRING;
	itemInfo.fState		= (bCanPaste)?0:MFS_GRAYED;
	itemInfo.dwTypeData	= L"Paste";
	itemInfo.cch		= 6;
	itemInfo.wID		= nIDOffset+(UINT)rClickEvents.size();
	rClickEvents.push_back(MenuClickPacket(this,&CPlisgoView::OnPaste));

	::InsertMenuItem(hMenu, nPos, TRUE, &itemInfo);
}


LRESULT		 CPlisgoView::DoContextMenu(IContextMenu* pIMenu, HMENU hMenu, int x, int y, std::vector<MenuClickPacket>& rClickEvents, const UINT nCustomIDOffset)
{
	LRESULT lr = 0;

	UINT nMenuSelection = TrackPopupMenu ( hMenu, TPM_LEFTBUTTON | TPM_RETURNCMD,
											x, y, 0, m_hWnd, NULL );

	if (nMenuSelection > 0)
	{
		WCHAR sBuffer[MAX_PATH];

		if (nMenuSelection < nCustomIDOffset)
		{
			--nMenuSelection;

			pIMenu->GetCommandString(nMenuSelection, GCS_VERBW, NULL, (CHAR*)sBuffer, MAX_PATH);

			if (wcscmp(sBuffer,L"explore") == 0)
			{
				int nItem = ListView_GetNextItem(m_hList, -1, LVNI_SELECTED);

				if (nItem != -1)
				{
					LPITEMIDLIST pIDL = ILCombine(m_pContainingFolder->GetIDList(), ListViewGetItemIDL(m_hList, nItem));

					if (pIDL != NULL)
					{
						m_ShellBrowser->BrowseObject(pIDL,SBSP_SAMEBROWSER);

						ILFree(pIDL);
					}
				}					
			}
			else if (wcscmp(sBuffer,L"rename") != 0)
			{
				CMINVOKECOMMANDINFOEX cmdInfo = {0};

				cmdInfo.cbSize = sizeof(CMINVOKECOMMANDINFOEX);
				cmdInfo.fMask = CMIC_MASK_UNICODE;
				cmdInfo.hwnd = m_hWnd;
				cmdInfo.lpVerb = (LPCSTR)(INT_PTR)(nMenuSelection);
				cmdInfo.lpDirectoryW = m_pContainingFolder->GetPath().c_str();
				cmdInfo.nShow = SW_SHOWNORMAL;

				lr = HRESULTTOLRESULT(pIMenu->InvokeCommand((CMINVOKECOMMANDINFO*)&cmdInfo));

				if (wcscmp(sBuffer,L"cut") == 0)
					MarkSelectedAsCut(m_hList);
			}
			else
			{
				ListView_EditLabel(m_hList, ListView_GetNextItem(m_hList, -1, LVIS_SELECTED));

				if (m_CommDlgBrowser.p != NULL)
					m_CommDlgBrowser->OnStateChange(this, CDBOSC_RENAME);
			}
		}
		else rClickEvents[nMenuSelection-nCustomIDOffset].Do();
	}

	return lr;
}


LRESULT		 CPlisgoView::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& rbHandled)
{
	CComPtr<ICommDlgBrowser2> pCommDlgBrowser2;

	if (m_CommDlgBrowser.p != NULL)
		m_CommDlgBrowser->QueryInterface(IID_ICommDlgBrowser2, (void**)&pCommDlgBrowser2);

	if (pCommDlgBrowser2.p != NULL)
		pCommDlgBrowser2->Notify(this, CDB2N_CONTEXTMENU_START);

	int   nSelItem = ListView_GetNextItem(m_hList,  -1, LVIS_SELECTED );
	int   x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

    if ( nSelItem != -1 && -1 == x  ||  -1 == y )
    {
        RECT rcIcon;

		ListView_GetItemRect(m_hList, nSelItem, &rcIcon, LVIR_ICON );

		::ClientToScreen(m_hList, (POINT*)&rcIcon );
		::ClientToScreen(m_hList, &((POINT*)&rcIcon)[1] );

        x = (rcIcon.right + rcIcon.left) / 2;
        y = (rcIcon.bottom + rcIcon.top) / 2;
    }


	DWORD nFlags =  CMF_NORMAL;

	IContextMenu* pIMenu = NULL;

	if (ListView_GetNextItem(m_hList, -1, LVNI_SELECTED) != -1)
	{
		GetItemObject(SVGIO_SELECTION, IID_IContextMenu, (void**)&pIMenu);

		nFlags |= CMF_CANRENAME | CMF_EXPLORE;
	}
	else GetItemObject(SVGIO_BACKGROUND, IID_IContextMenu, (void**)&pIMenu);
	
	
	LRESULT hResult = S_OK;
	
    HMENU hMenu = CreatePopupMenu();

	if (pIMenu != NULL)
	{
		if (m_IMenu2.p != NULL)
			m_IMenu2.Release();

		if (m_IMenu3.p != NULL)
			m_IMenu3.Release();

		if (pIMenu->QueryInterface(IID_IContextMenu3, (void**)&m_IMenu3) != S_OK)
			pIMenu->QueryInterface(IID_IContextMenu2, (void**)&m_IMenu2);
	
		const UINT nCustomIDOffset = HRESULT_CODE(pIMenu->QueryContextMenu(hMenu, 0, 1, 0x7fff, nFlags))+1;

		std::vector<MenuClickPacket> clickEvents;


		if (nFlags == CMF_NORMAL)
		{
			HMENU hViewSubMenu = CreatePopupMenu();

			InsertViewToMenu(hViewSubMenu, clickEvents, nCustomIDOffset, 0);

			MENUITEMINFO itemInfo = {0};

			itemInfo.cbSize		= sizeof(MENUITEMINFO);
			itemInfo.fMask		= MIIM_SUBMENU | MIIM_STRING;
			itemInfo.hSubMenu	= hViewSubMenu;
			itemInfo.dwTypeData	= L"View";
			itemInfo.cch		= 5;

			::InsertMenuItem(hMenu, 0, TRUE, &itemInfo);

			itemInfo.cbSize		= sizeof(MENUITEMINFO);
			itemInfo.fMask		= MIIM_TYPE;
			itemInfo.fType		= MF_SEPARATOR;

			::InsertMenuItem(hMenu, 1, TRUE, &itemInfo);

			InsertPasteToMenu(hMenu, clickEvents, nCustomIDOffset, FindSeperatorPosFromBottom(hMenu, 1));
		}
		

		DoContextMenu(pIMenu, hMenu, x, y, clickEvents, nCustomIDOffset);

		if (m_IMenu2.p != NULL)
			m_IMenu2.Release();

		if (m_IMenu3.p != NULL)
			m_IMenu3.Release();

		pIMenu->Release();
	}

	if (pCommDlgBrowser2.p != NULL)
		pCommDlgBrowser2->Notify(this, CDB2N_CONTEXTMENU_DONE);

	DestroyMenu(hMenu);

    return 0;
}


LRESULT		 CPlisgoView::FillInItem(LVITEM* pItem)
{
	if ( pItem->iItem < 0 || pItem->iItem > ListView_GetItemCount(m_hList))
		return S_FALSE;

	LPITEMIDLIST pIDL = (LPITEMIDLIST)pItem->lParam;
	
	if (pItem->iSubItem < 0)
		pItem->iSubItem = 0;
	
	LRESULT lr = 0;

	if (pItem->mask & LVIF_TEXT)
	{
		if (pItem->iSubItem < (int)m_nColumnIDMap.size())
		{
			HRESULT hr = m_pContainingFolder->GetTextOfColumn(pIDL, m_nColumnIDMap[pItem->iSubItem], pItem->pszText, pItem->cchTextMax);

			if (FAILED(hr))
				return HRESULTTOLRESULT(hr);
		}
		else
		{
			int nPlisgoColumnIndex = pItem->iSubItem-(int)m_nColumnIDMap.size();

			IPtrPlisgoFSRoot rootFSFolder = m_PlisgoFSFolder;

			assert(rootFSFolder.get() != NULL);

			if (nPlisgoColumnIndex < (int)rootFSFolder->GetColumnNum())
			{
				WCHAR sBuffer[MAX_PATH];

				lr = HRESULTTOLRESULT(m_pContainingFolder->GetItemName(pIDL, sBuffer, MAX_PATH));

				m_pAsynLoader->AddTextJob(pItem->iItem, pItem->iSubItem, sBuffer);
			}
			else lr = HRESULTTOLRESULT(E_UNEXPECTED);
		}
	}

	//Only do it when it's with text, as first entry is asked for many times as just image
	if ((pItem->mask == (LVIF_IMAGE|LVIF_TEXT)) && pItem->iImage == I_IMAGECALLBACK)
	{
		WCHAR sBuffer[MAX_PATH];

		HRESULT hr = m_pContainingFolder->GetItemName(pIDL, sBuffer, MAX_PATH);

		if (FAILED(hr))
			return HRESULTTOLRESULT(hr);

		const std::wstring sFile = m_pContainingFolder->GetPath() + L"\\" += sBuffer;

		//Add default icons
		if (GetFileAttributes(sFile.c_str()) & FILE_ATTRIBUTE_DIRECTORY)
			pItem->iImage = DEFAULTCLOSEDFOLDERICONINDEX;
		else
			pItem->iImage = DEFAULTFILEICONINDEX;

		m_pAsynLoader->AddIconJob(pItem->iItem, sBuffer);
	}

	return lr;
}



LRESULT		 CPlisgoView::OnGetDispInfo(int nCtrl, LPNMHDR pNmh, BOOL& rbHandled)
{
	NMLVDISPINFO* pNmv = CONTAINING_RECORD(pNmh, NMLVDISPINFO, hdr);

	rbHandled = TRUE;

	return FillInItem(&pNmv->item);
}


LRESULT		 CPlisgoView::OnLabelEdit(int nCtrl, LPNMHDR pNmh, BOOL& bHandled)
{
	NMLVDISPINFO* pNmv = CONTAINING_RECORD(pNmh, NMLVDISPINFO, hdr);

	if (pNmv->item.pszText == NULL)
		return S_OK;

	pNmv->item.mask = LVIF_PARAM;

	if (ListView_GetItem(m_hList, &pNmv->item))
	{
		LPITEMIDLIST pIDL = (LPITEMIDLIST)pNmv->item.lParam;

		LPITEMIDLIST pNewIDL;

		HRESULT hr = m_pContainingFolder->SetNameOf(m_hWnd, pIDL, pNmv->item.pszText, SHGDN_FOREDITING, &pNewIDL);
		
		if (hr != S_OK)
			return HRESULTTOLRESULT(hr);
		
		pNmv->item.lParam = (LPARAM)ILClone(ILFindLastID(pNewIDL));

		ILFree(pIDL);
		ILFree(pNewIDL);

		ListView_SetItem(m_hList, &pNmv->item);

		return S_OK;
	}
	
	return HRESULTTOLRESULT(E_UNEXPECTED);
}


LRESULT		 CPlisgoView::OnBeginDrag(int nCtrl, LPNMHDR pNmh, BOOL &rbHandled)
{
	CComObject<CPlisgoData>* pPlisgoDataObject;

    HRESULT hr = CComObject<CPlisgoData>::CreateInstance(&pPlisgoDataObject);

	if (FAILED(hr))
		return hr;

	pPlisgoDataObject->AddRef();

	pPlisgoDataObject->Init(this);

	CComQIPtr<IDataObject> pDataObject(pPlisgoDataObject);

	if (pDataObject.p != NULL)
	{
		CComObject<CPlisgoDropSource>* pPlisgoDropSource = NULL;

		hr = CComObject<CPlisgoDropSource>::CreateInstance(&pPlisgoDropSource);

		if (hr == S_OK)
		{
			pPlisgoDropSource->AddRef();

			CComQIPtr<IDropSource> pDropSource(pPlisgoDropSource);

			if (pDropSource.p != NULL)
			{
				DWORD dwEffect = (GetAsyncKeyState(VK_LSHIFT)|GetAsyncKeyState(VK_RSHIFT))?
								DROPEFFECT_MOVE:DROPEFFECT_COPY;

				hr = ::DoDragDrop( pDataObject, pDropSource, dwEffect, &dwEffect);
			}
			else hr = HRESULTTOLRESULT(E_NOINTERFACE);

			pPlisgoDropSource->Release();
		}
	}
	else hr = HRESULTTOLRESULT(E_NOINTERFACE);

	pPlisgoDataObject->Release();

	return hr;
}


static DWORD GetDropEffect(DWORD nKeyState)
{
	if (nKeyState&MK_SHIFT)
		return DROPEFFECT_MOVE;

	return DROPEFFECT_COPY;
}


STDMETHODIMP CPlisgoView::DragEnter( IDataObject* pDataObj, DWORD nKeyState, POINTL pt, DWORD *pnEffect)
{
	FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

	if( S_OK == pDataObj->QueryGetData(&fmt) )
	{
		m_nDropEffect = GetDropEffect(nKeyState);
	}

	*pnEffect = m_nDropEffect;

	return S_OK;
}


STDMETHODIMP CPlisgoView::DragOver( DWORD nKeyState, POINTL pt, DWORD *pnEffect)
{
	m_nDropEffect = GetDropEffect(nKeyState);

	*pnEffect = m_nDropEffect;
	return S_OK;
}


STDMETHODIMP CPlisgoView::DragLeave()
{
	m_nDropEffect = DROPEFFECT_NONE;
	return S_OK;
}


STDMETHODIMP CPlisgoView::Drop( IDataObject* pDataObj, DWORD nKeyState, POINTL pt, DWORD *pnEffect)
{
	FORMATETC fe1 = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

	STGMEDIUM stgmed;

	HRESULT hr = pDataObj->GetData(&fe1, &stgmed);

	if( hr == S_OK )
	{
		DoDrop((HDROP)stgmed.hGlobal, m_nDropEffect);

		if (pnEffect != NULL)
			*pnEffect = m_nDropEffect;

		::ReleaseStgMedium(&stgmed);
	}

	return hr;
}


LRESULT		 CPlisgoView::OnUpdateItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& rbHandled)
{
	rbHandled = TRUE;

	const LVITEM* pItem = (const LVITEM*)lParam;

	if (pItem != NULL || wParam >= sizeof(LVITEM))
	{
		if (pItem->iItem < (int)m_IconSources.size() )
		{
			if (pItem->mask & LVIF_IMAGE && m_IconList.get() != NULL)
				m_IconList->GetIconLocation(m_IconSources[pItem->iItem], pItem->iImage);

			ListView_SetItem(m_hList, pItem);

			ListView_RedrawItems(m_hList, pItem->iItem, pItem->iItem);
		}
		//else I'm guess it's been delayed and is late

		free((void*)pItem);
	}
	else
	{
		::UpdateWindow(m_hList);
	}

	return S_OK;
}