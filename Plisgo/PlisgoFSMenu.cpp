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
#include "PlisgoFSMenu.h"
#include "IconRegistry.h"


PlisgoFSMenu::PlisgoFSMenu(IPtrFSIconRegistry FSIcons, const std::wstring& rsPath, const WStringList& rSelection)
{
	m_sPath = rsPath;
	m_bCanUseClick = false;

	if (ReadTextFromFile(m_sText, (rsPath + L"\\.text").c_str()))
	{
		m_bEnabled = false;

		std::wstring sSelectionFile = rsPath + L"\\.selection";

		m_hSelectionFile = CreateFile(sSelectionFile.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (m_hSelectionFile == INVALID_HANDLE_VALUE)
			m_hSelectionFile = NULL;

		if (m_hSelectionFile != NULL)
		{
			m_bCanUseClick = true; //We has the lock

			for(WStringList::const_iterator it = rSelection.begin(); it != rSelection.end(); ++it)
			{
				LPCWSTR sPos = it->c_str();
				int nLength = (int)it->length();
		
				DWORD nWritten = 0;

				while(nLength)
				{
					char sBuffer[MAX_PATH];

					int nConverted = WideCharToMultiByte(CP_UTF8, 0, sPos, nLength, sBuffer, MAX_PATH-1, NULL, NULL);

					nWritten = 0;

					WriteFile(m_hSelectionFile, sBuffer, nConverted, &nWritten, NULL);

					nLength -= nWritten;
					sPos += nWritten;
				}

				nWritten = 0;

				WriteFile(m_hSelectionFile, "\r\n", 2, &nWritten, NULL);
			}
		
			FlushFileBuffers(m_hSelectionFile); //Mark selection writing is finished
		}
		else
		{
			if (GetFileAttributes(sSelectionFile.c_str()) == INVALID_FILE_ATTRIBUTES)
				m_bCanUseClick = true; //There is no lock not to have
		}


		int nEnabled = 0;

		if (ReadIntFromFile(nEnabled, (rsPath + L"\\.enabled").c_str()))
			m_bEnabled = (nEnabled != 0);
		else
			m_bEnabled = true;
	

		WIN32_FIND_DATAW	findData;

		HANDLE hFind = FindFirstFileW((m_sPath + L"\\.click.*").c_str(), &findData);

		if (hFind != NULL && hFind != INVALID_HANDLE_VALUE)
		{
			FindClose(hFind);

			if (wcsrchr(findData.cFileName, L'.') != findData.cFileName)
			{
				m_sClickCmd = m_sPath + L"\\" + findData.cFileName;

				m_sClickCmdArgs.empty();

				const size_t nBase = rsPath.rfind(L".plisgo");

				std::wstring sBase = rsPath.substr(0, nBase-1);

				for(WStringList::const_iterator it = rSelection.begin(); it != rSelection.end(); ++it)
				{
					m_sClickCmdArgs += L"\"";
					m_sClickCmdArgs += sBase;
					m_sClickCmdArgs += *it;
					m_sClickCmdArgs += L"\"";

					if (it != rSelection.end()-1)
						m_sClickCmdArgs+= L" ";
				}
			}
		}
			

		m_hIcon = NULL;

		if (m_bEnabled)
		{
			IconLocation iconLocation;

			if (FSIcons.get() != NULL)
			{
				if (FSIcons->ReadIconLocation(iconLocation, rsPath + L"\\.icon"))
					m_hIcon = ExtractIcon(g_hInstance, iconLocation.sPath.c_str(), iconLocation.nIndex);
			}

			LoadMenus(m_children, FSIcons, rsPath+L"\\", rSelection);
		}
	}
	else
	{
		m_hSelectionFile = NULL;
		m_bEnabled = true;
		m_hIcon = NULL;
	}
}


PlisgoFSMenu::~PlisgoFSMenu()
{
	if (m_hSelectionFile != NULL)
		CloseHandle(m_hSelectionFile);

	if (m_hIcon != NULL)
		DestroyIcon(m_hIcon);
}


int			PlisgoFSMenu::GetIndex() const
{
	const size_t nPos = m_sPath.rfind(L"\\.menu_");

	if (nPos == -1)
		return 0;

	return _wtoi(m_sPath.c_str() + nPos + 7 /*len("\\.menu_")*/);
}


void		PlisgoFSMenu::Click()
{
	if (m_bCanUseClick)
	{
		HANDLE hClickFile = CreateFile((m_sPath + L"\\.click").c_str(), GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hClickFile != NULL && hClickFile != INVALID_HANDLE_VALUE)
		{
			DWORD nWritten = 0;

			WriteFile(hClickFile, "1", 1, &nWritten, NULL);

			FlushFileBuffers(hClickFile);

			CloseHandle(hClickFile);
		}
	}

	if (m_sClickCmd.length())
		ShellExecute(NULL, NULL, m_sClickCmd.c_str(), m_sClickCmdArgs.c_str(), NULL, SW_SHOWNORMAL);
}


void		PlisgoFSMenu::LoadMenus(IPtrPlisgoFSMenuList&	rMenuList,
								  IPtrFSIconRegistry	FSIcons,
								  const std::wstring&	rsPath,
								  const WStringList&	rSelection)
{
	WIN32_FIND_DATA findData;

	std::wstring sTemp = rsPath + L".menu_*";

	HANDLE hFind = FindFirstFileW(sTemp.c_str(), &findData);

	if (hFind != NULL && hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			sTemp = rsPath + findData.cFileName;

			rMenuList.push_back(IPtrPlisgoFSMenu(new PlisgoFSMenu(FSIcons, sTemp, rSelection)));
		}
		while(FindNextFileW(hFind, &findData) != 0);

		FindClose(hFind);
	}

	std::sort(rMenuList.begin(), rMenuList.end());
	std::reverse(rMenuList.begin(), rMenuList.end());
}






bool		IPtrPlisgoFSMenu::operator>(const IPtrPlisgoFSMenu& rOther) const
{
	if (get() != NULL)
	{
		if (rOther.get() != NULL)
			return ((*this)->GetIndex() > rOther->GetIndex());
		else
			return true;
	}
	else return false;
}


bool		IPtrPlisgoFSMenu::operator>=(const IPtrPlisgoFSMenu& rOther) const
{
	if (get() != NULL)
	{
		if (rOther.get() != NULL)
			return ((*this)->GetIndex() >= rOther->GetIndex());
		else
			return true;
	}
	else return false;
}


bool		IPtrPlisgoFSMenu::operator == (const IPtrPlisgoFSMenu& rOther) const
{
	if (get() != NULL)
	{
		if (rOther.get() != NULL)
			return ((*this)->GetIndex() == rOther->GetIndex());
		else
			return false;
	}
	else return (rOther.get() == NULL);
}


bool		IPtrPlisgoFSMenu::operator < (const IPtrPlisgoFSMenu& rOther) const
{
	if (get() != NULL)
	{
		if (rOther.get() != NULL)
			return ((*this)->GetIndex() < rOther->GetIndex());
		else
			return false;
	}
	else return true;
}


bool		IPtrPlisgoFSMenu::operator <= (const IPtrPlisgoFSMenu& rOther) const
{
	if (get() != NULL)
	{
		if (rOther.get() != NULL)
			return ((*this)->GetIndex() <= rOther->GetIndex());
		else
			return false;
	}
	else return true;
}


bool		IPtrPlisgoFSMenu::operator != (const IPtrPlisgoFSMenu& rOther) const
{
	if (get() != NULL)
	{
		if (rOther.get() != NULL)
			return ((*this)->GetIndex() != rOther->GetIndex());
		else
			return true;
	}
	else return (rOther.get() != NULL);
}