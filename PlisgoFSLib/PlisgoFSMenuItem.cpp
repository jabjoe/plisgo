/*
	Copyright 2009 Eurocom Entertainment Software Limited

    This file is part of PlisgoFSLib.

    Eurocom Entertainment Software Limited, hereby disclaims all
    copyright interest in “PlisgoFSLib” written by Joe Burmeister.

    PlisgoFSLib is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

    PlisgoFSLib is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
	License along with PlisgoFSLib.  If not, see
	<http://www.gnu.org/licenses/>.
*/

#include "PlisgoFSLib.h"
#include "PlisgoFSMenuItem.h"
#include "PlisgoFSHiddenFolders.h"




class SelectionFile;

class EnabledFile : public PlisgoFSStringFile
{
public:
	
	EnabledFile(boost::shared_ptr<SelectionFile>	selectionFile,
				IPtrFileEvent						enabledEvent);

	virtual int		Open(	DWORD				nDesiredAccess,
							DWORD				nShareMode,
							DWORD				nCreationDisposition,
							DWORD				nFlagsAndAttributes,
							IPtrPlisgoFSData&	rData);
private:
	IPtrFileEvent						m_EnabledEvent;
	boost::shared_ptr<SelectionFile>	m_selectionFile;
};


class SelectionFile : public PlisgoFSStringFile
{
public:

	SelectionFile(RootPlisgoFSFolder* pOwner)
	{
		m_pOwner = pOwner;
	}

	bool		CallPerPath(FileEvent& rEvent);

private:
	RootPlisgoFSFolder*	m_pOwner;
};


class PlisgoMiscClickData : public PlisgoFSUserData
{
public:

	PlisgoMiscClickData() { m_bPendingClick = false; }

	static PlisgoMiscClickData*	Get(IPtrPlisgoFSData& rData)
	{
		PlisgoMiscClickData* pClick = dynamic_cast<PlisgoMiscClickData*>(rData.get());

		assert(pClick != NULL);

		return pClick;
	}

	bool m_bPendingClick;
};



IPtrPlisgoFSUserData	PlisgoMiscClickFile::CreateData()
{
	IPtrPlisgoFSUserData result = boost::make_shared<PlisgoMiscClickData>();

	assert(result.get() != NULL);

	return result;
}


int	PlisgoMiscClickFile::Write(	LPCVOID				pBuffer,
								DWORD				nNumberOfBytesToWrite,
								LPDWORD				pnNumberOfBytesWritten,
								LONGLONG			nOffset,
								IPtrPlisgoFSData&	rData)
{
	int nResult = PlisgoFSStringFile::Write(	pBuffer,
												nNumberOfBytesToWrite,
												pnNumberOfBytesWritten,
												nOffset,
												rData);
	if (nResult == 0)
		PlisgoMiscClickData::Get(rData)->m_bPendingClick = true;

	return nResult;
}


int	PlisgoMiscClickFile::FlushBuffers(IPtrPlisgoFSData&	rData)
{
	PlisgoMiscClickData* pData = PlisgoMiscClickData::Get(rData);

	if (pData->m_bPendingClick)
	{
		Triggered();

		pData->m_bPendingClick = false;
	}

	return PlisgoFSStringFile::FlushBuffers(rData);
}


int	PlisgoMiscClickFile::Close(IPtrPlisgoFSData& rData)
{
	if (PlisgoMiscClickData::Get(rData)->m_bPendingClick)
		FlushBuffers(rData);

	return PlisgoFSStringFile::Close(rData);
}





class ClickFile : public PlisgoMiscClickFile
{
public:
	
	ClickFile(	boost::shared_ptr<SelectionFile>	selectionFile,
				IPtrFileEvent						onClickEvent) : m_OnClickEvent(onClickEvent)
	{
		m_selectionFile = selectionFile;
	}

	virtual void Triggered()
	{
		if (m_OnClickEvent.get() != NULL)
		{
			assert(m_selectionFile.get() != NULL);

			m_selectionFile->CallPerPath(*m_OnClickEvent);
		}
	}

private:
	IPtrFileEvent						m_OnClickEvent;
	boost::shared_ptr<SelectionFile>	m_selectionFile;
};






EnabledFile::EnabledFile(	boost::shared_ptr<SelectionFile>	selectionFile,
							IPtrFileEvent						enabledEvent) : PlisgoFSStringFile("0")
{
	m_EnabledEvent = enabledEvent;
	m_selectionFile = selectionFile;
}


int		EnabledFile::Open(	DWORD				nDesiredAccess,
							DWORD				nShareMode,
							DWORD				nCreationDisposition,
							DWORD				nFlagsAndAttributes,
							IPtrPlisgoFSData&	rData)
{
	if (m_EnabledEvent.get() == NULL || m_selectionFile->CallPerPath(*m_EnabledEvent))
		SetString("1");
	else
		SetString("0");

	return PlisgoFSStringFile::Open(nDesiredAccess, nShareMode, nCreationDisposition, nFlagsAndAttributes, rData);
}


static WCHAR sSeperators[] = L";|\r\n";

static LPCWSTR FindNextSeperator(LPCWSTR sPaths)
{
	assert(sPaths != NULL);

	for(int n = 0; n < (sizeof(sSeperators)/sizeof(WCHAR))-1; ++n)
	{
		LPCWSTR sResult = wcschr(sPaths,sSeperators[n]);

		if (sResult != NULL)
			return sResult;
	}

	return NULL;
}


static LPCWSTR SkipSeperators(LPCWSTR sPaths)
{
	assert(sPaths != NULL);

	for(; sPaths[0] != L'\0'; ++sPaths)
	{
		bool bIsSeperator = false;

		for (int n = 0; n < (sizeof(sSeperators)/sizeof(WCHAR))-1; ++n)
			if (*sPaths == sSeperators[n])
			{
				bIsSeperator =true;
				break;
			}

		if (!bIsSeperator)
			break;
	}
	
	return sPaths;
}



bool	SelectionFile::CallPerPath(FileEvent& rEvent)
{
	bool bResult = false;

	std::wstring sPaths;
	
	GetWideString(sPaths);

	LPCWSTR sCStr		= sPaths.c_str();

	LPCWSTR sNextSeperator = FindNextSeperator(sCStr);

	const std::wstring& rsBase	= m_pOwner->GetPath();
	IPtrPlisgoVFS vfs			= m_pOwner->GetVFS();

	while(sNextSeperator != NULL)
	{
		const WCHAR temp = *sNextSeperator;

		*const_cast<WCHAR*>(sNextSeperator) = 0;

		std::wstring sPath = rsBase + sCStr;

		IPtrPlisgoFSFile file = vfs->TracePath(sPath.c_str());
		
		if (file.get() != NULL)
			bResult |= rEvent.Do(file, sPath);

		*const_cast<WCHAR*>(sNextSeperator) = temp;

		const LPCWSTR sNewCStr = SkipSeperators(sNextSeperator);

		if (sNewCStr == sCStr)
			++sCStr;
		else
			sCStr = sNewCStr;

		sNextSeperator = FindNextSeperator(sCStr);
	}

	if (sCStr[0] != '\0')
	{
		std::wstring sPath = rsBase + sCStr;

		IPtrPlisgoFSFile file = vfs->TracePath(sPath.c_str());
		
		if (file.get() != NULL)
			bResult |= rEvent.Do(file, sPath);
	}

	bResult |= rEvent.Do(IPtrPlisgoFSFile(), rsBase); //Inform event object list is complete.
	
	return bResult;
}





PlisgoFSMenuItem::PlisgoFSMenuItem(	RootPlisgoFSFolder*	pOwner,
									IPtrFileEvent		onClickEvent,
									IPtrFileEvent		enabledEvent,
									LPCWSTR				sUserText,
									int					nIconList,
									int					nIconIndex)
{
	if (sUserText != NULL)
	{
		AddChild(L".text", IPtrPlisgoFSFile(new PlisgoFSStringReadOnly(sUserText)));

		if (nIconList != -1 && nIconIndex != -1)
		{
			boost::format fmt = boost::format("%1% : %2%") %nIconList %nIconIndex;

			AddChild(L".icon", IPtrPlisgoFSFile(new PlisgoFSStringReadOnly(fmt.str())));
		}
		
		boost::shared_ptr<SelectionFile> selectionFile(new SelectionFile(pOwner));

		AddChild(L".selection", selectionFile);

		if (enabledEvent.get() != NULL)
			AddChild(L".enabled", IPtrPlisgoFSFile(new EnabledFile(selectionFile, enabledEvent)));

		if (onClickEvent.get() != NULL)
			AddChild(L".click", IPtrPlisgoFSFile(new ClickFile(selectionFile, onClickEvent)));
	}
}
