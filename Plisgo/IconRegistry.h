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

#define DEFAULTFILEICONINDEX			0
#define DEFAULTCLOSEDFOLDERICONINDEX	1
#define DEFAULTOPENFOLDERICONINDEX		2


struct IconLocation
{
	IconLocation()	{ nIndex = 0; }
	IconLocation(const std::wstring& rsPath, const int _nIndex)	{ sPath = rsPath; nIndex = _nIndex; }

	std::wstring	sPath;
	int				nIndex;

	bool IsValid() const { return (sPath.length() > 0); }
};



class FSIconRegistry
{
	friend class IconRegistry;
public:

	FSIconRegistry(	LPCWSTR				sFSName,
					int					nVersion,
					IconRegistry*		pMain,
					const std::wstring& rsRootPath);

	const IconRegistry*		GetMainIconRegistry() const		{ return m_pMain; }


	const std::wstring&		GetFSName() const				{ return m_sFSName; }
	int						GetFSVersion() const			{ return m_nVersion; }

	bool					ReadIconLocation(IconLocation& rIconLocation, const std::wstring& rsPligoFile, const ULONG nHeight);

	bool					GetIconLocation(IconLocation& rIconLocation, UINT nList, UINT nIndex, const ULONG nHeight) const;
	bool					GetIconLocation(IconLocation& rIconLocation, const IconLocation& rBase, IPtrRefIconList iconList, UINT nOverlayList, UINT nOverlayIndex) const;

	bool					GetFSIcon(HICON& rhIcon, UINT nList, UINT nIndex, UINT nHeight) const;

protected:
	
	void					AddInstancePath(std::wstring sRootPath);

private:

	bool					LookupKey(std::wstring& rsPath, const std::wstring& rsKey) const;

	bool					GetFSIcon_RW(HICON& rhIcon, UINT nList, UINT nIndex, UINT nHeight);
	const std::wstring&		GetInstancePath_RW();


	struct VersionImageList
	{
		VersionImageList()
		{
			nHeight = 0;
		}

		VersionImageList(UINT _nHeight)
		{
			nHeight = _nHeight;
		}

		CComPtr<IImageList>	ImageList;
		UINT				nHeight;
	};

	typedef std::vector<VersionImageList>						VersionedImageList;
	typedef std::tr1::unordered_map<std::wstring, std::wstring>	BurnReadyCached;

	bool					GetBestImageList_RO(VersionedImageList::const_iterator& rBestFit, UINT nList, UINT nHeight) const;
	bool					GetBestImageList_RW(VersionedImageList::const_iterator& rBestFit, UINT nList, UINT nHeight);


	std::wstring							m_sFSName;
	int										m_nVersion;

	WStringList								m_Paths;

	std::vector<VersionedImageList>			m_ImageLists;
	BurnReadyCached							m_BurnReadyCached;

	IconRegistry*							m_pMain;
	mutable boost::shared_mutex				m_Mutex;
};




class RefIconList
{
	friend class FSIconRegistry;
public:

	RefIconList(UINT nHeight);

	HIMAGELIST				GetImageList() const				{ return reinterpret_cast<HIMAGELIST>(m_ImageList.p); }
	UINT					GetHeight() const					{ return m_nHeight; }

	bool					GetFileIconLocation(IconLocation& rIconLocation, const std::wstring& rsPath) const;
	bool					GetFolderIconLocation(IconLocation& rIconLocation, bool bOpen) const;

	bool					GetIcon(HICON& rhResult, const int nIndex) const;


	bool					GetIconLocation(IconLocation& rIconLocation, const int nIndex) const;
	bool					GetIconLocationIndex(int& rnIndex, const IconLocation& rIconLocation) const;

	void					RemoveOlderThan(ULONG64 n100ns);

private:

	bool					AddEntry_RW(int& rnIndex, HICON hIcon);
	bool					GetIconLocationIndex_RW(int& rnIndex, const IconLocation& rIconLocation, const std::wstring& rsKey);

	int						GetFreeSlot();

	struct CachedIcon
	{
		IconLocation		Location;
		ULONG64				nLastModified;
		volatile ULONG64	nTime;

		bool				IsValid() const;
	};

	typedef std::tr1::unordered_map<std::wstring,int>		CachedIconsMap;


	mutable boost::shared_mutex		m_Mutex;
	UINT							m_nHeight;
	CComPtr<IImageList>				m_ImageList;
	std::map<std::wstring, int>		m_Extensions;
	std::map<int, std::wstring>		m_ExtensionsInverse;
	std::vector<bool>				m_EntryUsed;
	std::vector<CachedIcon>			m_CachedIcons;
	CachedIconsMap					m_CachedIconsMap;
	ULONG64							m_nLastOldestEntry;
};



class IconRegistry
{
public:

	IconRegistry();

	IPtrFSIconRegistry		GetFSIconRegistry(LPCWSTR sFS, int nVersion, std::wstring& rsFirstPath) const;

	IPtrRefIconList			GetRefIconList(ULONG nHeight) const;

private:

	std::map<std::wstring, boost::weak_ptr<FSIconRegistry> >	m_FSIconRegistries;
	std::vector<boost::weak_ptr<RefIconList> >					m_IconLists;
	mutable boost::shared_mutex									m_Mutex;
};

