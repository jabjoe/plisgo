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

#pragma once

#include "PlisgoFSFiles.h"

class RootPlisgoFSFolder;


class PlisgoMiscClickFile : public PlisgoFSStringFile
{
public:

	PlisgoMiscClickFile() : PlisgoFSStringFile("0") {}

	virtual void Triggered() = 0;

	virtual int	Write(	LPCVOID		pBuffer,
						DWORD		nNumberOfBytesToWrite,
						LPDWORD		pnNumberOfBytesWritten,
						LONGLONG	nOffset,
						ULONGLONG*	pInstanceData);

	
	virtual int	FlushBuffers(ULONGLONG* pInstanceData);

	virtual int	Close(ULONGLONG* pInstanceData);
};



class PlisgoFSMenuItem : public PlisgoFSStorageFolder
{
public:

	PlisgoFSMenuItem(	RootPlisgoFSFolder*	pOwner,
						IPtrFileEvent		onClickEvent,
						IPtrFileEvent		enabledEvent,
						LPCWSTR				sUserText,
						int					nIconList = -1,
						int					nIconIndex = -1);
};