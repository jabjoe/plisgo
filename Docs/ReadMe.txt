Plisgo
=======



Introduction
=============


Plisgo is an Explorer namespace which creates custom overlays, icons, thumbnails, menus and columns  from a virtual filesystem. It affectively redirects the namespace definition to the filesystem.


The reason why you would do this are many, but here are a few.

Each Explorer namespace can and will be loaded by any process that calls up a open/save dialog and the user browses into the namespace. This has two big consequences. First, a namespace should be lightweight. Second, partly related to the first, a namespace should not be written in .NET. Only a single .NET runtime can be loaded for a process, some .NET runtimes have compatible bytecodes, but which .NET runtime is loaded first cannot be controlled. MS themselves advise against writing any shell extensions in .NET for this reason (though they still do it from time to time).

With Plisgo, everything is redirected to the filesystem. This keeps the namespace small, and means, as the filesystem can be written in anything, the developer is free to work in whatever language suits them.

Writing an Explorer namespace is well known to be a minefield, with Plisgo, you have a (hopefully) simple API to do your shell UI.




History
=======
Plisgo was originally written by Joe Burmeister internally to Eurocom to provide a UI for a internal virtual filesystem.
As it could be useful to others and others working on it, or with it, is useful to Eurocom Entertainment Software Limited, it has been agreed to open Plisgo to the outside world.



Contents
========


Provided in this package should be:


* "ReadMe.txt"			File	- This text file.

* "Plisgo"			Folder	- C++ Source code for the Pligo Explorer namespace.

* "Plisgo.dll"			File	- Compiled DLL of Plisgo Explorer namespace.

* "PlisgoFSLib"			Folder	- C++ Source code for of an example of a PlisgoFS API implimentation.

* "PlisgoFSLib.dll"		File	- C DLL of PlisgoFSLib functions, almost every language can import and use C functions from a DLL.

* "PlisgoFSLib.h"		File	- C header for the PlisgoFSLib.dll

* "PlisgoFS_Folder.pdf"		File	- Scalable Vector Graphics diagram of PlisgoFS folder/API.

* "PlisgoFS_Folder_Ex.pdf"	File	- Scalable Vector Graphics diagram of PlisgoFS folder/API extentions.

* "TestFolder.zip"		File	- contains "real" filesystem mock up of a PlisgoFS filesystem.
(Mark folder as read only from command line using "ATTRIB +r testfolder", Explorer's read only doesn't seam to be same......)


* "Plisgo_Dokan_Example"	Folder	- C++ Source code for a Dokan/Plisgo example done using PlisgoFSLib.

* "Plisgo_Dokan_Example.exe"	File	- Compiled executable of Dokan/Plisgo example.



Requiments to run Plisgo_Dokan_Example
=======================================

* Dokan Installed
(http://dokan-dev.net/en/download/)

* Visual Studio 2008 runtimes installed.	(http://www.microsoft.com/DOWNLOADS/details.aspx?FamilyID=9b2da534-3e03-4391-8a4d-074b9f2bc1bf)



Install Plisgo
==============

From the command prompt, browse to where you have unzipped this folder and do "regsvr32 bin\Win32\plisgo.dll".
On a 64bit Windows install also do "regsvr32 bin\x64\plisgo.dll". (You need both unless you really aren't running any 32bit applications (but you will be))



Uninstalling Plisgo
===================

As above, but do "regsvr32 /u plisgo.dll".



Build Requiments
================

* Visual Studio 2008
* Boost 1.44 (or better)   (add the boost include/lib directories to the Visual Studio VC++ include/lib Directories)




Licences
========


The actural Plisgo Explorer namespace is GPLv3.
The example PlisgoFSLib client API implimentation is LGPLv3.
Plisgo_Dokan_Example is under the BSD license, do what you want with it.

Eurocom Entertainment Software Limited, hereby disclaims all copyright interest in “Plisgo” written by Joe Burmeister.



Contact
=======


Joe Burmeister - plisgo@eurocom.co.uk