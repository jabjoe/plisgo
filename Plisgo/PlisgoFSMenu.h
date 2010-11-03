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

typedef std::vector<IPtrPlisgoFSMenu>	IPtrPlisgoFSMenuList;


class PlisgoFSMenu
{
public:

	PlisgoFSMenu(IPtrFSIconRegistry FSIcons, const std::wstring& rsPath, const WStringList& rSelection);
	~PlisgoFSMenu();

	static	void				LoadMenus(IPtrPlisgoFSMenuList& rMenuList, IPtrFSIconRegistry FSIcons, const std::wstring& rsPath, const WStringList& rSelection);

	bool						IsEnabled() const		{ return m_bEnabled; }

	HICON						GetIcon() const			{ return m_hIcon; }
	const std::wstring&			GetText() const			{ return m_sText; }
	int							GetIndex() const;

	void						Click();

	const IPtrPlisgoFSMenuList&	GetChildren() const		{ return m_children; }
	
private:
	std::wstring			m_sPath;
	bool					m_bEnabled;
	HICON					m_hIcon;
	std::wstring			m_sText;
	HANDLE					m_hSelectionFile;
	bool					m_bCanUseClick;
	std::wstring			m_sClickCmd;
	std::wstring			m_sClickCmdArgs;
	IPtrPlisgoFSMenuList	m_children;
};

class IPtrPlisgoFSMenu : public boost::shared_ptr<PlisgoFSMenu>
{
public:
	IPtrPlisgoFSMenu() : boost::shared_ptr<PlisgoFSMenu>() {}
	IPtrPlisgoFSMenu(PlisgoFSMenu* pPlisgoFSMenu)	: boost::shared_ptr<PlisgoFSMenu>(pPlisgoFSMenu) {}

	bool operator > (const IPtrPlisgoFSMenu& rOther) const;
	bool operator >= (const IPtrPlisgoFSMenu& rOther) const;
	bool operator == (const IPtrPlisgoFSMenu& rOther) const;
	bool operator < (const IPtrPlisgoFSMenu& rOther) const;
	bool operator <= (const IPtrPlisgoFSMenu& rOther) const;
	bool operator != (const IPtrPlisgoFSMenu& rOther) const;

};