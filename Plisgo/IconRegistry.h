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

	bool	IsValid() const { return (sPath.length() > 0); }
	ULONG64	GetHash() const;
};



class FSIconRegistry
{
	friend class FSIconRegistriesManager;
public:

	FSIconRegistry(	LPCWSTR				sFSName,
					int					nVersion,
					IconRegistry*		pMain,
					IPtrPlisgoFSRoot&	rRoot);

	~FSIconRegistry();

	const std::wstring&		GetFSName() const				{ return m_sFSName; }
	int						GetFSVersion() const			{ return m_nVersion; }

	bool					ReadIconLocation(IconLocation& rIconLocation, const std::wstring& rsPath) const;

protected:
	
	void					AddReference(IPtrPlisgoFSRoot& rRoot);

	bool					HasInstancePath();

private:

	bool					GetInstancePath_Locked(std::wstring& rsResult);

	bool					GetIconLocation(IconLocation& rIconLocation, UINT nList, UINT nIndex) const;
	bool					GetIconLocation_Locked(IconLocation& rIconLocation, UINT nList, UINT nIndex);

	class LoadedImageList
	{
	public:
		bool	Init(const std::wstring& rsFile);
		bool	IsValid() const;
		void	Clear();

		bool	IsIconFile() const	{ return (m_nFileType == 1); }
		bool	IsCodeFile() const	{ return (m_nFileType == 2); }

		const std::wstring& GetFilePath() const { return m_sFile; }
		ULONG64		GetLastModTime() const		{ return m_nLastModTime; }

		HIMAGELIST	GetImageList() const		{ return m_hImageList; }

	private:

		HIMAGELIST		m_hImageList;
		std::wstring	m_sFile;
		ULONG32			m_nFileType;
		ULONG64			m_nLastModTime;
	};

	typedef std::map<UINT, LoadedImageList>		VersionedImageList;


	class CombiImageListIcon
	{
	public:
		bool	Init(const WStringList& rFiles);
		bool	IsValid() const;
		void	Clear();

		bool	GetIconLocation(IconLocation& rIconLocation, UINT nIndex, bool& rbLoaded) const;
		bool	CreateIconLocation(IconLocation& rIconLocation, UINT nIndex, IconRegistry* pIconRegistry);


	private:
		VersionedImageList			m_ImageLists;
		std::vector<IconLocation>	m_Baked;
	};


	typedef std::map<ULONG, CombiImageListIcon>						CombiImageListIconMap;


	std::wstring							m_sFSName;
	int										m_nVersion;

	typedef std::vector<boost::weak_ptr<PlisgoFSRoot> >		References;

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

	struct IconSource
	{
		IconLocation	location;
		ULONG64			nLastModTime;
	};

	bool					GetCreatedIconPath(std::wstring& rsIconPath, const std::vector<IconSource>& rSources, bool* pbIsCurrent = NULL) const;

	bool					MakeOverlaid(	IconLocation&		rDst,
											const IconLocation& rFirst,	
											const IconLocation& rSecond) const;

	bool					GetAsWindowsIconLocation(IconLocation& rIconLocation, const std::wstring& rsImageFile, UINT nHeightHint = 0) const;

	bool					GetWindowsIconLocation(IconLocation& rIconLocation, const IconLocation& rSrcIconLocation, UINT nHeightHint = 0) const;


private:

	struct CreatedIcon
	{
		std::wstring			sResult;
		std::vector<IconSource> sources;

		bool	IsValid() const;
	};

	typedef boost::unordered_map<std::wstring, CreatedIcon>			CreatedIcons;

	CreatedIcons									m_CreatedIcons;
	std::wstring									m_sShellPath;
	mutable boost::shared_mutex						m_Mutex;
};


class FSIconRegistriesManager
{
public:

	static FSIconRegistriesManager*	GetSingleton();


	IPtrFSIconRegistry		GetFSIconRegistry(LPCWSTR sFS, int nVersion, IPtrPlisgoFSRoot& rRoot) const;
	void					ReleaseFSIconRegistry(IPtrFSIconRegistry& rFSIconRegistry, IPtrPlisgoFSRoot& rRoot); 

	IconRegistry*			GetIconRegistry() 	{ return &m_IconRegistry; }

private:

	FSIconRegistriesManager()	{}

	IPtrFSIconRegistry		GetFSIconRegistry_Locked(const std::wstring& rsKey, LPCWSTR sFS, int nVersion, IPtrPlisgoFSRoot& rRoot);

	typedef boost::unordered_map<std::wstring, boost::weak_ptr<FSIconRegistry> >	FSIconRegistries;

	FSIconRegistries				m_FSIconRegistries;
	IconRegistry					m_IconRegistry;
	mutable boost::shared_mutex		m_Mutex;
};
