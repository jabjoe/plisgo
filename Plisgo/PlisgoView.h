/*
    Copyright 2009 Eurocom Entertainment Software Limited

    This file is part of Plisgo.

    Eurocom Entertainment Software Limited, hereby disclaims all
    copyright interest in �Plisgo� written by Joe Burmeister.

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



#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

#include "IconRegistry.h"

extern const CLSID	CLSID_PlisgoView;

const UINT MSG_UPDATELISTITEM = (WM_USER + 0x01);

class CPlisgoFolder;

// CPlisgoView

class ATL_NO_VTABLE CPlisgoView :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPlisgoView, &CLSID_PlisgoView>,
	public IShellView2,
    public IOleCommandTarget,
	public IDropTarget,
	public ATL::CWindowImpl<CPlisgoView>
{
public:
    CPlisgoView();
    ~CPlisgoView();


BEGIN_COM_MAP(CPlisgoView)
    COM_INTERFACE_ENTRY(IShellView)
    COM_INTERFACE_ENTRY(IShellView2)
    COM_INTERFACE_ENTRY(IDropTarget)
    COM_INTERFACE_ENTRY(IOleCommandTarget)
    COM_INTERFACE_ENTRY(IOleWindow)
END_COM_MAP()

	DECLARE_REGISTRY_RESOURCEID(IDR_PLISGOVIEW)

//For break points
	static HRESULT InternalQueryInterface(void* pThis,
										  const _ATL_INTMAP_ENTRY* pEntries,
										  REFIID iid,
										  void** ppvObject )
	{
		return CComObjectRootBase::InternalQueryInterface(pThis, pEntries, iid, ppvObject);
	}

protected:

	BOOL	MessageProcessHock(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT& rResult);

BEGIN_MSG_MAP(CPlisgoView)

	if (MessageProcessHock(hWnd, uMsg, wParam, lParam, lResult))
		return TRUE;

    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)

    NOTIFY_CODE_HANDLER(LVN_DELETEITEM, OnListDeleteitem)
    NOTIFY_CODE_HANDLER(HDN_ITEMCLICK, OnHeaderItemclick)
    NOTIFY_CODE_HANDLER(NM_SETFOCUS, OnListSetfocus)
    NOTIFY_CODE_HANDLER(NM_KILLFOCUS, OnListKillfocus)
    NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDefaultCommand)
    NOTIFY_CODE_HANDLER(NM_RETURN, OnDefaultCommand)
	
	NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnGetDispInfo)
	NOTIFY_CODE_HANDLER(LVN_ENDLABELEDIT, OnLabelEdit)
	NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
	NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnItemActivated)

    MESSAGE_HANDLER(WM_PLISGOVIEWSHELLMESSAGE, OnCustomViewShellMessage)
    MESSAGE_HANDLER(WM_INITMENUPOPUP, OnInitMenuPopup)

    NOTIFY_CODE_HANDLER(LVN_BEGINDRAG, OnBeginDrag)
    NOTIFY_CODE_HANDLER(LVN_BEGINRDRAG, OnBeginDrag)

	MESSAGE_HANDLER(MSG_UPDATELISTITEM, OnUpdateItem)

END_MSG_MAP()

public:
    // IOleWindow
    STDMETHOD(GetWindow)(HWND* phWnd);
    STDMETHOD(ContextSensitiveHelp)(BOOL)	{ return E_NOTIMPL; }


    // IShellView methods
    STDMETHOD(CreateViewWindow)(LPSHELLVIEW, LPCFOLDERSETTINGS, LPSHELLBROWSER, LPRECT, HWND*);
    STDMETHOD(DestroyViewWindow)();
    STDMETHOD(GetCurrentInfo)(LPFOLDERSETTINGS);
    STDMETHOD(Refresh)();
    STDMETHOD(UIActivate)(UINT);

    STDMETHOD(AddPropertySheetPages)(DWORD, LPFNADDPROPSHEETPAGE, LPARAM)	{ return E_NOTIMPL; }
    STDMETHOD(EnableModeless)(BOOL)											{ return E_NOTIMPL; }
    STDMETHOD(SaveViewState)()												{ return E_NOTIMPL; }
    STDMETHOD(SelectItem)(LPCITEMIDLIST pIDLItem, UINT uFlags);
    STDMETHOD(GetItemObject)(UINT uItem, REFIID rIID, LPVOID* pPv);
    STDMETHOD(TranslateAccelerator)(LPMSG pMsg);

    // IShellView2 methods

	STDMETHOD(CreateViewWindow2)(LPSV2CVW2_PARAMS lpParams);
	STDMETHOD(GetView)(SHELLVIEWID* pVID, ULONG uView);
	STDMETHOD(HandleRename)(PCUITEMID_CHILD pIDLNew)									{ return E_NOTIMPL; }
	STDMETHOD(SelectAndPositionItem)(PCUITEMID_CHILD pIDL, UINT uFlags, POINT* pPoint)	{ return E_NOTIMPL; }

    // IOleCommandTarget methods
    STDMETHOD(QueryStatus)(const GUID*, ULONG, OLECMD prgCmds[], OLECMDTEXT*);
    STDMETHOD(Exec)(const GUID*, DWORD, DWORD, VARIANTARG*, VARIANTARG*);

	// IDropTarget
    STDMETHOD(DragEnter)( IDataObject* pDataObj, DWORD nKeyState, POINTL pt, DWORD *pnEffect);
    STDMETHOD(DragOver)( DWORD nKeyState, POINTL pt, DWORD *pnEffect);
    STDMETHOD(DragLeave)();
    STDMETHOD(Drop)( IDataObject* pDataObj, DWORD nKeyState, POINTL pt, DWORD *pnEffect);


    // Message handlers
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& rbHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& rbHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& rbHandled);
    LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& rbHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& rbHandled);
    LRESULT OnInitMenuPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& rbHandled);
    LRESULT OnMenuSelect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& rbHandled);
	LRESULT OnCustomViewShellMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& rbHandled);

    // List control message handlers
    LRESULT OnListDeleteitem(int nCtrl, LPNMHDR pNmh, BOOL& rbHandled);
    LRESULT OnHeaderItemclick(int nCtrl, LPNMHDR pNmh, BOOL& rbHandled);
    LRESULT OnListSetfocus(int nCtrl, LPNMHDR pNmh, BOOL& rbHandled);
    LRESULT OnListKillfocus(int nCtrl, LPNMHDR pNmh, BOOL& rbHandled);
	LRESULT OnGetDispInfo(int nCtrl, LPNMHDR pNmh, BOOL& rbHandled);
	LRESULT OnLabelEdit(int nCtrl, LPNMHDR pNmh, BOOL& rbHandled);
	LRESULT OnItemChanged(int nCtrl, LPNMHDR pNmh, BOOL& rbHandled);
	LRESULT OnItemActivated(int nCtrl, LPNMHDR pNmh, BOOL& rbHandled);
	LRESULT	OnDefaultCommand(int nCtrl, LPNMHDR pNmh, BOOL& rbHandled);

	//Drag-and-drop
	LRESULT OnBeginDrag(int nCtrl, LPNMHDR pNmh, BOOL &rbHandled);

    // Other stuff
    bool Init( CPlisgoFolder* pContainingFolder );

	HGLOBAL		GetSelectionAsDropData();

    LRESULT OnUpdateItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& rbHandled);
	

protected:
	static LRESULT CALLBACK ListWndProcedure(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);

	typedef void (CPlisgoView::*PlisgoViewMenuItemClickCB)();

	void	DoDrop(HDROP hDrop, DWORD nDropEffect);
	void	OnPaste();
	void	OnSelectAll();
	void	InvertSelection();
	void	OnThumbnailsViewMenuItemClick();
	void	OnLargeViewMenuItemClick();
	void	OnDetailsViewMenuItemClick();

	void	DoUndo();
	void	PutSelectedToClipboard(const bool bMove);

	HRESULT	RenameSelection();
	HRESULT	DeleteSelection();

	bool	IsPasteAvailable();
	DWORD	GetAttributesOfSelection();

	BOOL	DoStandardCommand(UINT nID);

	void	InsertViewToMenu(HMENU hMenu, int nPos);
	void	InsertPasteToMenu(HMENU hMenu, int nPos);

	LRESULT	DoContextMenu(IContextMenu* pIMenu, HMENU hMenu, int x, int y);

	static int CALLBACK DefaultCompareItems(LPARAM l1, LPARAM l2, LPARAM lData);

	void	GetSelection(boost::shared_ptr<LPCITEMIDLIST>& rSelection, int& rnSelected, LPARAM nItemFlag);
	void	GetSelection(WStringList& rSelection);

	LRESULT FillInItem(LVITEM* pItem);
    void	FillList();
	void	HandleActivate(UINT nState);
    void	HandleDeactivate();
	void	RefreshIconList(LONG nHeight);

	void	RefreshItemImages();
	void	AddItem(LPITEMIDLIST pIDL);
	bool	DeleteItem(LPITEMIDLIST pIDL);


	class AsynLoader
	{
	public:
		AsynLoader(HWND hWnd, IPtrRefIconList iconList, int nColumnOffset, IPtrPlisgoFSRoot plisgoFSFolder, const std::wstring& rsPath);
		~AsynLoader();

		void	AddIconJob(int nItem, LPCWSTR sName);
		void	AddTextJob(int nItem, int nSubItem, LPCWSTR sName);

		void	ClearJobs();

	protected:

		void	DoWork();

		static DWORD WINAPI ThreadProcCB(LPVOID lpParameter);


	private:

		void	WakeupWorker();

		struct Job
		{
			int				nItem;
			int				nSubItem;
			WCHAR			sName[MAX_PATH];
		};

		void	DoJob(const Job& rJob);

		HANDLE						m_hThread;
		DWORD						m_nThreadID;
		HWND						m_hWnd;
		IPtrRefIconList				m_IconList;
		IPtrPlisgoFSRoot			m_PlisgoFSFolder;
		int							m_nColumnOffset;
		volatile bool				m_bAlive;
		HANDLE						m_hEvent;
		std::wstring				m_sPath;
		boost::mutex				m_Mutex;

		volatile LONG				m_nSpinLock;
		std::queue<Job>				m_Jobs;
	};


	AsynLoader*								m_pAsynLoader;

	CComPtr<IContextMenu2>					m_IMenu2;
	CComPtr<IContextMenu3>					m_IMenu3;
	CComPtr<ICommDlgBrowser2>				m_CommDlgBrowser;

	ULONG									m_nShellNotificationID;

	IPtrRefIconList							m_IconList;

	volatile LONG							m_bViewInit;

	std::vector<LPITEMIDLIST>				m_InitSelection;

	CPlisgoFolder*							m_pContainingFolder;
	IPtrPlisgoFSRoot						m_PlisgoFSFolder;

	std::vector<int>						m_nColumnIDMap;
	std::vector<IconLocation>				m_IconSources;
    HWND									m_hWndParent;
    FOLDERSETTINGS							m_FolderSettings;
    CComPtr<IShellBrowser>					m_ShellBrowser;
	HMENU									m_hMenu;
    HWND									m_hList;
	UINT									m_nUIState;
	int										m_nSortedColumn;
	bool									m_bForwardSort;
	DWORD									m_nDropEffect;
};

