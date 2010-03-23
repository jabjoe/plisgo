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

#include "IconRegistry.h"
#include "PlisgoFSMenu.h"


class PlisgoFSRoot
{
public:

	PlisgoFSRoot(const std::wstring& rsPath, IconRegistry* pIconRegistry);
	~PlisgoFSRoot();

	ULONG64				GetTimeSinceLastUse();

	LPCWSTR				GetFSName() const								{ return m_sFSName.c_str(); }
	ULONG				GetFSVersion() const							{ return m_nFSVersion; }
	const std::wstring& GetPath() const									{ return m_sPath; }

	enum ColumnAlignment
	{
		LEFT	= -1,
		CENTER	= 0,
		RIGHT	= 1
	};

	enum ColumnType
	{
		TEXT	= 0, //default
		INT		= 1,
		FLOAT	= 2
	};

	ULONG				GetColumnNum() const							{ return (ULONG)m_Columns.size(); }
	const std::wstring&	GetColumnHeader(int nIndex) const				{ return m_Columns[nIndex].sHeader; }
	ColumnAlignment		GetColumnAlignment(int nIndex) const			{ return m_Columns[nIndex].eAlignment; }
	ColumnType			GetColumnType(int nIndex) const					{ return m_Columns[nIndex].eType; }
	int					GetColumnDefaultWidth(int nIndex) const			{ return m_Columns[nIndex].nWidth; }


	bool				IsStandardColumnDisabled(int nStdColumn) const	{ return m_DisabledStandardColumn[nStdColumn]; }

	IPtrFSIconRegistry	GetFSIconRegistry() const						{ return m_IconRegistry; }

	bool				GetFolderIcon(IconLocation& rIconLocation, const UINT nHeight, bool bOpen = false) const;
	bool				GetExtensionIcon(IconLocation& rIconLocation, LPCWSTR sExt, const UINT nHeight) const;


	void				GetMenuItems(IPtrPlisgoFSMenuList& rMenus, const WStringList& rSelection);

	bool				GetPathColumnTextEntry(std::wstring& rsResult, const std::wstring& rsPath, int nIndex) const;
	bool				GetPathColumnIntEntry(int& rnResult, const std::wstring& rsPath, int nIndex) const;
	bool				GetPathColumnFloatEntry(double& rnResult, const std::wstring& rsPath, int nIndex) const;

	int					GetPathIconIndex(const std::wstring& rsPath, IPtrRefIconList iconList, bool bOpen = false) const;

	bool				GetPathIconLocation(IconLocation& rIconLocation, const std::wstring& rsPath, IPtrRefIconList iconList, bool bOpen = false) const;

private:

	bool				GetPathColumnEntryPath(std::wstring& rsOutPath, const std::wstring& rsInPath, int nIndex) const;

	bool				GetThumbnailFile(	std::wstring&		rsFile,
											const std::wstring& rsShellInfoFolder,
											const std::wstring& rsName) const;

	bool				GetCustomIconIconLocation(	IconLocation&		rIconLocation,
													const std::wstring& rsShellInfoFolder,
													const std::wstring& rsName,
													const UINT			nHeigh) const;

	bool				GetThumbnailIconLocation(	IconLocation&		rIconLocation,
													const std::wstring& rsShellInfoFolder,
													const std::wstring& rsName,
													const UINT			nHeight) const;

	bool				GetMountIconLocation(	IconLocation&		rIconLocation,
												const std::wstring& rsPath,
												IPtrRefIconList		iconList,
												bool				bOpen = false) const;

	bool				GetShellInfoFolder(	std::wstring&		rsName,
											std::wstring&		rsShellInfoFolder,
											const std::wstring& rsPath) const;


	std::wstring							m_sFSName;
	int										m_nFSVersion;
	std::wstring							m_sPath;
	bool									m_DisabledStandardColumn[8];

	struct ColumnDef
	{
		ColumnDef() { eAlignment = PlisgoFSRoot::CENTER; eType = PlisgoFSRoot::TEXT; nWidth =-1; }

		std::wstring	sHeader;
		ColumnAlignment	eAlignment;
		ColumnType		eType;
		int				nWidth;
	};

	std::vector<ColumnDef>					m_Columns;

	IPtrFSIconRegistry						m_IconRegistry;
};

