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
	IconLocation()	{ nIndex = 0; nSrcList = nSrcIndex = nFSIconRegID = -1; }
	IconLocation(const std::wstring& rsPath, const int _nIndex)	{ sPath = rsPath; nIndex = _nIndex; }

	std::wstring	sPath;
	int				nIndex;
	UINT			nSrcList,nSrcIndex; //Trace from original FSIconRegistry source
	UINT			nFSIconRegID;

	bool	IsValid() const { return (sPath.length() > 0); }
	ULONG64	GetHash() const;
};



class FSIconRegistry
{
	friend class FSIconRegistriesManager;
public:

	FSIconRegistry(	LPCWSTR				sFSName,
					int					nFSVersion,
					IconRegistry*		pMain,
					IPtrPlisgoFSRoot&	rRoot,
					UINT				nFSIconRegID);

	~FSIconRegistry();

	const std::wstring&		GetFSName() const				{ return m_sFSName; }
	int						GetFSVersion() const			{ return m_nFSVersion; }

	bool					ReadIconLocation(IconLocation& rIconLocation, const std::wstring& rsPath) const;

protected:
	
	void					AddReference(IPtrPlisgoFSRoot& rRoot);

	bool					HasInstancePath();

private:

	bool					GetInstancePath_Locked(std::wstring& rsResult);

	bool					GetIconLocation(IconLocation& rIconLocation, UINT nList, UINT nIndex) const;
	bool					GetIconLocation_RLocked(IconLocation& rIconLocation, UINT nList, UINT nIndex, bool bWrite = false) const;
	bool					GetIconLocation_RWLocked(IconLocation& rIconLocation, UINT nList, UINT nIndex);

	class LoadedImageList
	{
	public:
		bool	Init(const std::wstring& rsFile);
		void	Clear();

		bool	IsIconFile() const	{ return (m_nFileType == 1); }
		bool	IsCodeFile() const	{ return (m_nFileType == 2); }

		const std::wstring& GetFilePath() const { return m_sFile; }

		HIMAGELIST	GetImageList() const		{ return m_hImageList; }

	private:

		HIMAGELIST		m_hImageList;
		std::wstring	m_sFile;
		ULONG32			m_nFileType;
	};

	typedef std::map<UINT, LoadedImageList>		VersionedImageList;


	class CombiImageListIcon
	{
	public:
		bool	Init(const WStringList& rFiles);
		void	Clear();

		bool	GetIconLocation(IconLocation& rIconLocation, UINT nIndex, bool& rbLoaded) const;
		bool	CreateIconLocation(IconLocation& rIconLocation, UINT nList, UINT nIndex, UINT nFSRegId, IconRegistry* pIconRegistry);


	private:
		VersionedImageList			m_ImageLists;
		std::vector<IconLocation>	m_Baked;
	};


	typedef std::map<ULONG, CombiImageListIcon>						CombiImageListIconMap;


	std::wstring							m_sFSName;
	int										m_nFSVersion;
	UINT									m_nFSIconRegID;

	//We use the actural pointer as a GUID for the map to ensure the same one doesn't get stored more than once.
	typedef std::map<boost::uint64_t, boost::weak_ptr<PlisgoFSRoot> >		References;

	References								m_References;

	CombiImageListIconMap					m_ImageLists;

	IconRegistry*							m_pMain;
	mutable boost::shared_mutex				m_Mutex;
};



class IconRegistry
{
public:

	IconRegistry();

	bool					GetFileIconLocation(IconLocation& rIconLocation, const std::wstring& rsPath) const;
	bool					GetFolderIconLocation(IconLocation& rIconLocation, bool bOpen) const;

	bool					GetDefaultOverlayIconLocation(IconLocation& rIconLocation, const std::wstring& rsName) const;

	bool					MakeOverlaid(	IconLocation&		rDst,
											const IconLocation& rFirst,	
											const IconLocation& rSecond) const;

	bool					GetAsWindowsIconLocation(IconLocation& rIconLocation, const std::wstring& rsImageFile, UINT nHeightHint = 0) const;

	bool					GetWindowsIconLocation(IconLocation& rIconLocation, const IconLocation& rSrcIconLocation, UINT nHeightHint = 0) const;

private:

	std::wstring					m_sShellPath;
};


class FSIconRegistriesManager
{
public:

	static FSIconRegistriesManager*	GetSingleton();


	IPtrFSIconRegistry		GetFSIconRegistry(LPCWSTR sFS, int nFSVersion, IPtrPlisgoFSRoot& rRoot) const;
	IPtrFSIconRegistry		GetFSIconRegistry(UINT nID) const;
	void					ReleaseFSIconRegistry(IPtrFSIconRegistry& rFSIconRegistry, IPtrPlisgoFSRoot& rRoot); 

	IconRegistry*			GetIconRegistry() 	{ return &m_IconRegistry; }

private:

	FSIconRegistriesManager()	{ }

	bool					GetFSIconRegistry_Locked(IPtrFSIconRegistry& rResult, const std::wstring& rsKey, IPtrPlisgoFSRoot& rRoot) const;
	IPtrFSIconRegistry		CreateFSIconRegistry_Locked(const std::wstring& rsKey, LPCWSTR sFS, int nFSVersion, IPtrPlisgoFSRoot& rRoot);

	typedef boost::unordered_map<std::wstring, boost::weak_ptr<FSIconRegistry> >	FSIconRegistriesMap;

	FSIconRegistriesMap								m_FSIconRegistriesMap;
	IconRegistry									m_IconRegistry;
	mutable boost::shared_mutex						m_Mutex;
	std::vector<boost::weak_ptr<FSIconRegistry> >	m_FSIconRegistries;
};
