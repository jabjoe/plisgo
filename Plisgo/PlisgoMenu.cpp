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
#include "PlisgoMenu.h"
#include "PlisgoFSFolderReg.h"

// CPlisgoMenu

int gShellIDList = RegisterClipboardFormat(CFSTR_SHELLIDLIST);

#define HIDA_GetPIDLFolder(pida) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define HIDA_GetPIDLItem(pida, i) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])



/*
	We don't know if the icons in the treenode have changed, so assume they have.
	This is a hack, but it works and is fast.
*/

static BOOL			GetTreeItemName(HWND hWnd, HTREEITEM hItem, WCHAR* sBuffer, int nBufferSize)
{
	TVITEMEX item = {0};

	item.mask		= TVIF_HANDLE|TVIF_TEXT;
	item.pszText	= sBuffer;
	item.cchTextMax	= nBufferSize;
	item.hItem		= hItem;

	return TreeView_GetItem(hWnd, &item);
}



static HTREEITEM	GetChildByName(LPCWSTR sName, HWND hWnd, HTREEITEM hItem, size_t nNameLength = -1)
{
	HTREEITEM hChild = TreeView_GetChild(hWnd, hItem);

	WCHAR sBuffer[MAX_PATH];

	while(hChild != NULL)
	{
		GetTreeItemName(hWnd, hChild, sBuffer, MAX_PATH);

		bool bMatch = true;

		if (_wcsnicmp(sBuffer, sName, nNameLength) == 0)
			return hChild;
		else
			hChild = TreeView_GetNextSibling(hWnd, hChild);
	}

	return NULL;
}


static HTREEITEM	FollowRelativePath(std::wstring::const_iterator itPath, HWND hWnd, HTREEITEM hItem)
{
	if (itPath._Myptr[0] == L'\0')
		return hItem;

	while(itPath._Myptr[0] != L'\0' && *itPath == L'\\')
		++itPath;

	if (itPath._Myptr[0] == L'\0')
		return hItem;

	LPCWSTR sPathPos = itPath._Myptr;//&(*itPath);

	LPCWSTR sSlash = wcschr(sPathPos, L'\\');
	
	if (sSlash == NULL)
		return GetChildByName(sPathPos, hWnd, hItem);

	size_t nSlash = sSlash-sPathPos;

	HTREEITEM hChild = GetChildByName(sPathPos, hWnd, hItem, nSlash);

	if (hChild == NULL)
		return NULL;
	
	return FollowRelativePath(itPath + nSlash+1, hWnd, hChild);
}



static HTREEITEM	FindDrive(HTREEITEM hParent, WCHAR sDrive[], HWND hWnd, int nDepth)
{
	if (nDepth > 3) //Don't go crazy, no point going all the way down!
		return NULL;

	TVITEM item = {0};

	item.mask = TVIF_CHILDREN|TVIF_HANDLE;
	item.hItem = hParent;


	if (!TreeView_GetItem(hWnd, &item))
		return NULL;

	if (item.cChildren == I_CHILDRENCALLBACK)
		return NULL;


	HTREEITEM hChild = TreeView_GetChild(hWnd, hParent);

	WCHAR sBuffer[MAX_PATH];

	while(hChild != NULL)
	{
		GetTreeItemName(hWnd, hChild, sBuffer, MAX_PATH);

		if (wcsstr(sBuffer, sDrive) != NULL)
			return hChild;

		HTREEITEM hResult = FindDrive(hChild, sDrive, hWnd, nDepth+1);

		if (hResult != NULL)
			return hResult;

		hChild = TreeView_GetNextSibling(hWnd, hChild);
	}

	return NULL;
}


static HTREEITEM	FindDrive(WCHAR nDrive, HWND hWnd)
{
	HTREEITEM hRoot = TreeView_GetRoot(hWnd);

	WCHAR sDrive[] = {L'(',toupper(nDrive),L':',L')',L'\0'};

	while(hRoot != NULL)
	{
		HTREEITEM hResult = FindDrive(hRoot, sDrive, hWnd, 0);

		if (hResult != NULL)
			return hResult;

		hRoot = TreeView_GetNextSibling(hWnd, hRoot);
	}

	return NULL;
}


struct TreeSelectionPacket
{
	std::map<HWND, std::wstring>	SelectionMap;
	std::wstring					sBase;
};


BOOL CALLBACK	StoreSelectionCB(HWND hWnd, LPARAM lParam)
{
	WCHAR sBuffer[MAX_PATH] = {0};

	::GetClassName(hWnd, sBuffer, MAX_PATH);

	if (wcscmp(L"SysTreeView32", sBuffer) == 0)
	{
		HTREEITEM hRoot = TreeView_GetRoot(hWnd);

		if (hRoot == NULL)
			return FALSE;

		TreeSelectionPacket* pPacket = (TreeSelectionPacket*)lParam;

		HTREEITEM hDrive = FindDrive(pPacket->sBase[0], hWnd);

		if (hDrive == NULL)
			return FALSE;

		HTREEITEM hSelection = TreeView_GetSelection(hWnd);

		if (hSelection == NULL)
			return FALSE;

		std::wstring sSelection;

		while(hSelection != NULL && hSelection != hDrive)
		{
			WCHAR sBuffer[MAX_PATH];

			GetTreeItemName(hWnd, hSelection, sBuffer, MAX_PATH);

			wcscat_s(sBuffer, MAX_PATH, L"\\");

			sSelection.insert(0, sBuffer);

			hSelection = TreeView_GetParent(hWnd, hSelection);
		}

		if (hSelection == hDrive)
		{
			WCHAR sDrive[] = L" :\\";

			sDrive[0] = pPacket->sBase[0];

			sSelection.insert(0, sDrive);
			sSelection.resize(sSelection.length()-1); //Remove trailing slash

			pPacket->SelectionMap[hWnd] = sSelection;
		}

		return FALSE;
	}

	return TRUE;
}


BOOL CALLBACK	RestoreSelectionCB(HWND hWnd, LPARAM lParam)
{
	WCHAR sBuffer[MAX_PATH] = {0};

	::GetClassName(hWnd, sBuffer, MAX_PATH);

	if (wcscmp(L"SysTreeView32", sBuffer) == 0)
	{
		TreeSelectionPacket* pPacket = (TreeSelectionPacket*)lParam;

		std::map<HWND, std::wstring>::const_iterator it = pPacket->SelectionMap.find(hWnd);

		if (it == pPacket->SelectionMap.end())
			return FALSE;

		HTREEITEM hRoot = TreeView_GetRoot(hWnd);

		if (hRoot == NULL)
			return FALSE;

		HTREEITEM hDrive = FindDrive(pPacket->sBase[0], hWnd);

		if (hDrive == NULL)
			return FALSE;

		HTREEITEM hFolder = hDrive;

		if (it->second.length() > 3)
			hFolder = FollowRelativePath(it->second.begin()+3, hWnd, hDrive);

		if (hFolder != NULL)
			TreeView_SelectItem(hWnd, hFolder);

		return FALSE;
	}

	return TRUE;
}




struct AsyncClickPacket
{
	IPtrPlisgoFSMenu	MenuItem;
	WStringList			Selection;
	std::wstring		sBasePath;
};


void __cdecl AsyncClickPacketCB( AsyncClickPacket* pPacket )
{
	pPacket->MenuItem->Click();

	WStringList selection = pPacket->Selection;

	std::wstring sBasePath = pPacket->sBasePath;

	delete pPacket;
	pPacket = NULL;

	WStringList folders;

	CComPtr<IShellFolder> pShellDesktop = NULL;

	if (!SUCCEEDED(SHGetDesktopFolder(&pShellDesktop)))
		return;

	boost::trim_right_if(sBasePath, boost::is_any_of(L"\\"));

	TreeSelectionPacket packet;

	packet.sBase = sBasePath;

	HWND hExplorer = FindWindowEx(GetDesktopWindow(), NULL, L"ExploreWClass", NULL);

	while(hExplorer != NULL)
	{
		EnumChildWindows(hExplorer, StoreSelectionCB, (LPARAM)&packet);

		hExplorer = FindWindowEx(GetDesktopWindow(), hExplorer, L"ExploreWClass", NULL);
	}

	for(WStringList::iterator it = selection.begin(); it != selection.end(); ++it)
	{
		it->insert(0, sBasePath);

		if (GetFileAttributes(it->c_str()) & FILE_ATTRIBUTE_DIRECTORY)
		{
			//It's worse case, but it acturally works

			if (it->length() > 3) //But don't do this nastiness with drives.
			{
				SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHW, it->c_str(), NULL);
				SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW|SHCNF_FLUSH, it->c_str(), NULL);
			}
		}
		else SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW|SHCNF_FLUSH, it->c_str(), NULL);
	}

	assert(selection.size());

	size_t nSlash = selection[0].rfind(L'\\');

	assert(nSlash != -1);

	SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATHW|SHCNF_FLUSH, selection[0].substr(0, nSlash).c_str(), NULL);

	hExplorer = FindWindowEx(GetDesktopWindow(), NULL, L"ExploreWClass", NULL);

	while(hExplorer != NULL)
	{
		EnumChildWindows(hExplorer, RestoreSelectionCB, (LPARAM)&packet);

		hExplorer = FindWindowEx(GetDesktopWindow(), hExplorer, L"ExploreWClass", NULL);
	}
}







STDMETHODIMP CPlisgoMenu::Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID)
{
	m_Selection.clear();

	HRESULT hr = S_FALSE;

	std::wstring sParentPath;


	if (pDataObj != NULL)
	{
		STGMEDIUM medium;
		FORMATETC fmte = {(CLIPFORMAT)gShellIDList, (DVTARGETDEVICE FAR *)NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		
		hr = pDataObj->GetData(&fmte, &medium);

		if (SUCCEEDED(hr))
		{
			if (medium.hGlobal != NULL)
			{
				CIDA* pCIDA = (CIDA*)GlobalLock(medium.hGlobal);

				LPCITEMIDLIST pIDFolder = HIDA_GetPIDLFolder(pCIDA);

				GetWStringPathFromIDL(sParentPath, pIDFolder);


				if (sParentPath.length())
				{
					if (isalpha(sParentPath[0]) && sParentPath.length() > 1 && sParentPath[1] == L':')
					{
						if (*(sParentPath.end()-1) != L'\\')
							sParentPath += L"\\";
					}
					else sParentPath.clear(); //It's not a path
				}

				m_RootFSFolder = PlisgoFSFolderReg::GetSingleton()->GetPlisgoFSRoot(sParentPath.c_str());

				for(UINT n = 0; n < pCIDA->cidl; ++n)
				{
					LPCITEMIDLIST pIDLChild = HIDA_GetPIDLItem(pCIDA, n);

					PIDLIST_ABSOLUTE pFullIDL = ILCombine(pIDFolder, pIDLChild);

					std::wstring sFullPath;

					GetWStringPathFromIDL(sFullPath, pFullIDL);

					ILFree(pFullIDL);

					if (sFullPath.length() < 1 || !isalpha(sFullPath[0]) || sFullPath[1] != L':')
						continue; //It's not a path, skip it.

					IPtrPlisgoFSRoot root = PlisgoFSFolderReg::GetSingleton()->GetPlisgoFSRoot(sFullPath.c_str());

					if (m_RootFSFolder != NULL)
					{
						if (root == NULL || root == m_RootFSFolder)
						{
							m_Selection.push_back(sFullPath.substr(m_RootFSFolder->GetPath().length()-1));
						}
						else if (root != NULL && !m_Selection.size() && pCIDA->cidl == 1)
						{
							m_RootFSFolder = root;
							m_Selection.push_back(sFullPath.substr(root->GetPath().length()-1));
							break;
						}
					}
					else if (root != NULL)
					{
						m_RootFSFolder = root;
						m_Selection.push_back(sFullPath.substr(root->GetPath().length()-1));
						break;
					}
				}
				
				GlobalUnlock(medium.hGlobal);
			}
			else hr = S_FALSE;

			ReleaseStgMedium ( &medium );
		}
	}

	return hr;
}


STDMETHODIMP CPlisgoMenu::InvokeCommand( LPCMINVOKECOMMANDINFO pICI)
{
	if (m_RootFSFolder.get() == NULL)
		return E_NOTIMPL;

	UINT nCmdID = LOWORD(pICI->lpVerb) + m_nBeginID;

	std::map<UINT, IPtrPlisgoFSMenu>::const_iterator it = m_nIDToPlisgoMenu.find(nCmdID);

	if (it == m_nIDToPlisgoMenu.end())
		return E_INVALIDARG;

	//Do click asyncronousely so if it waits on explorer at all (message pump) there isn't dead lock.
	AsyncClickPacket* pPacket = new AsyncClickPacket();

	if (pPacket != NULL)
	{
		pPacket->MenuItem	= it->second;
		pPacket->Selection	= m_Selection;
		pPacket->sBasePath	= m_RootFSFolder->GetPath();
		
		_beginthread((void (__cdecl *)(void *))AsyncClickPacketCB, 0, pPacket);
	}

	return S_OK;
}


bool		 CPlisgoMenu::InsertPlisgoMenuToMenu(HMENU hMenu, const IPtrPlisgoFSMenu& rPlisgoMenu, UINT& rnID, int nPos)
{
	if (rPlisgoMenu.get() == NULL || hMenu == NULL || !rPlisgoMenu->IsEnabled())
		return false;

	const std::wstring& rsText = rPlisgoMenu->GetText();

	if (rsText.length() == 0)
	{
		if (!(GetMenuState(hMenu, nPos, MF_BYPOSITION) & MF_SEPARATOR))
			InsertMenu(hMenu, nPos, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);

		return true;
	}

	HMENU hSubMenu = NULL;

	MENUITEMINFO itemInfo = {0};

	itemInfo.cbSize		= sizeof(MENUITEMINFO);
	itemInfo.wID		= rnID;
	itemInfo.dwItemData = (ULONG_PTR)rPlisgoMenu.get();

	if (rsText.length() != 0)
	{
		itemInfo.fMask		= MIIM_FTYPE|MIIM_STRING|MIIM_ID|MIIM_STATE|MIIM_DATA;
		itemInfo.fType		= MF_STRING;
		itemInfo.fState		= MFS_ENABLED;
		itemInfo.dwTypeData	= (LPWSTR)rsText.c_str();
		itemInfo.cch		= (UINT)rsText.length();
	}
	else
	{
		itemInfo.fMask		= MIIM_FTYPE;
		itemInfo.fType		= MFT_SEPARATOR;
	}

	if (rPlisgoMenu->GetIcon() != NULL)
	{
		itemInfo.fMask |= MIIM_BITMAP;
		itemInfo.hbmpItem =  HBMMENU_CALLBACK; /*future note: TortoiseSVN give a bitmap under Vista (6.0 and above)*/
	}

	const IPtrPlisgoFSMenuList& rChildren = rPlisgoMenu->GetChildren();

	if (rChildren.size())
	{
		hSubMenu = CreateMenu();
		itemInfo.hSubMenu = hSubMenu;
		itemInfo.fMask |= MIIM_SUBMENU;
	}


	m_nIDToPlisgoMenu[rnID++] = rPlisgoMenu;
	InsertMenuItem(hMenu, nPos, TRUE, &itemInfo);

	if (rChildren.size())
	{
		int nEnabledChildren = 0;

		for(IPtrPlisgoFSMenuList::const_iterator it = rChildren.begin(); it != rChildren.end(); ++it)
			if (InsertPlisgoMenuToMenu(hSubMenu, *it, rnID, 0))
				++nEnabledChildren;

		if (nEnabledChildren == 0)
		{
			DestroyMenu(hSubMenu);

			RemoveMenu(hMenu, nPos, MF_BYPOSITION);

			m_nIDToPlisgoMenu.erase(--rnID);

			return false;
		}
	}

	return true;
}


STDMETHODIMP CPlisgoMenu::QueryContextMenu(	HMENU hMenu,
											UINT nIndexMenu,
											UINT nCmd,
											UINT /*nCmdLast*/,
											UINT uFlags)
{
	if (m_RootFSFolder.get() == NULL)
		return E_NOTIMPL;

	m_nBeginID = nCmd++;

	m_RootFSFolder->GetMenuItems(m_PlisgoFSMenuItems, m_Selection);

	if (m_PlisgoFSMenuItems.size() == 0)
		return E_NOTIMPL;

	UINT nStartPos = nIndexMenu;

	UINT nAddMenuItems = 0;

	for(IPtrPlisgoFSMenuList::const_iterator it = m_PlisgoFSMenuItems.begin(); it != m_PlisgoFSMenuItems.end(); ++it)
		if (InsertPlisgoMenuToMenu(hMenu, *it, nCmd, nIndexMenu++))
			++nAddMenuItems;

	if (nAddMenuItems)
	{
		InsertMenu(hMenu, nIndexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);
		InsertMenu(hMenu, nStartPos, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);
		++nCmd;
	}

	m_nEndID = nCmd;

	return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(m_nEndID - m_nBeginID)));
}



STDMETHODIMP CPlisgoMenu::HandleMenuMsg2( UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	if (pResult != NULL)
		*pResult = FALSE;

	if (m_RootFSFolder.get() == NULL)
		return E_NOTIMPL;

	if (nMsg == WM_MEASUREITEM)
	{
		MEASUREITEMSTRUCT* pMIS = (MEASUREITEMSTRUCT*)lParam;

		if (pMIS != NULL)
		{
			const UINT nIconHeight = TOPOWEROFTWO(GetSystemMetrics(SM_CYMENUCHECK) + GetSystemMetrics(SM_CYEDGE));

			pMIS->itemWidth += GetSystemMetrics(SM_CYEDGE)*2;

			if (pMIS->itemHeight < nIconHeight)
				pMIS->itemHeight = nIconHeight;

			if (pResult != NULL)
				*pResult = TRUE;
		}
	}
	else if (nMsg == WM_DRAWITEM && wParam == 0)
	{
		DRAWITEMSTRUCT* pDrawItemPacket = (DRAWITEMSTRUCT*)lParam;

		if (pDrawItemPacket != NULL && pDrawItemPacket->CtlType == ODT_MENU &&
			pDrawItemPacket->itemID > m_nBeginID &&
			pDrawItemPacket->itemID < m_nEndID)
		{
			PlisgoFSMenu* pPlisgoFSMenu = (PlisgoFSMenu*)pDrawItemPacket->itemData;

			const int nIconHeight = TOPOWEROFTWO(GetSystemMetrics(SM_CYMENUCHECK) + GetSystemMetrics(SM_CYEDGE));

			DrawIconEx(	pDrawItemPacket->hDC,
						pDrawItemPacket->rcItem.left - nIconHeight,
						pDrawItemPacket->rcItem.top + (pDrawItemPacket->rcItem.bottom - pDrawItemPacket->rcItem.top - nIconHeight) / 2,
						pPlisgoFSMenu->GetIcon(), nIconHeight, nIconHeight, 0, NULL, DI_NORMAL);

			if (pResult != NULL)
				*pResult = TRUE;
		}
	}

	return S_OK;
}
