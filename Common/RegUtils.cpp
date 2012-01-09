
#include "Utils.h"
#include "RegUtils.h"


bool	OpenKeyQuery(HKEY& rhKey, HKEY hRootKey, LPCWSTR sKey, bool bDoWOW64, bool bEnum = false)
{
	DWORD	nFlags	= KEY_QUERY_VALUE;

	if (bDoWOW64)
		nFlags |= KEY_WOW64_64KEY;

	if (bEnum)
		nFlags |= KEY_ENUMERATE_SUB_KEYS;

	return (RegOpenKeyEx(hRootKey, sKey, 0, nFlags, &rhKey) == ERROR_SUCCESS);
}



bool	LoadDWORDFromReg(HKEY hRootKey, LPCWSTR sKey, LPCWSTR sName, DWORD& rnValue, bool bDoWOW64 )
{
	HKEY	hKey = NULL;

	if (!OpenKeyQuery(hKey, hRootKey, sKey, bDoWOW64))
		return false;

	int		nNumber		= 0;
	DWORD	nBufSize	= sizeof(nNumber);
	LONG	nResult		= RegQueryValueEx(hKey, sName, 0, NULL, (BYTE*)&nNumber, &nBufSize);

	RegCloseKey(hKey);

	rnValue = nNumber;

	return (nResult == ERROR_SUCCESS);
}


bool	SaveDWORDToReg(HKEY hRootKey, LPCWSTR sKey, LPCWSTR sName, DWORD nValue, bool bDoWOW64 )
{
	HKEY	hKey	= NULL;
	DWORD	nFlags	= KEY_WRITE;

	if (bDoWOW64)
		nFlags |= KEY_WOW64_64KEY;

	if (RegCreateKeyEx(hRootKey, sKey, 0, NULL, REG_OPTION_NON_VOLATILE, nFlags, NULL, &hKey, NULL) != ERROR_SUCCESS)
		return false;

	LONG	nResult = RegSetValueEx(hKey, sName, 0, REG_DWORD, (BYTE*)&nValue, sizeof(nValue));

	RegCloseKey(hKey);

	return (nResult == ERROR_SUCCESS);
}


bool	LoadStringFromReg(HKEY hRootKey, LPCWSTR sKey, LPCWSTR sName, std::wstring& rsValue, bool bDoWOW64 )
{
	HKEY	hKey = NULL;

	if (!OpenKeyQuery(hKey, hRootKey, sKey, bDoWOW64))
		return false;

	bool	bResult		= false;
	DWORD	nBufSize	= 0;

	RegQueryValueEx(hKey, sName, 0, NULL, NULL, &nBufSize);

	if (nBufSize > 0)
	{
		rsValue.assign(nBufSize/2,L' '); // /2 because it's wchar not char

		RegQueryValueEx(hKey, sName, 0, NULL, (LPBYTE)rsValue.c_str(), &nBufSize);

		//Remove any extra terminations

		while(rsValue.length() && rsValue[rsValue.length()-1] == '\0')
			rsValue.resize(rsValue.length()-1);

		bResult = true;
	}
	
	RegCloseKey(hKey);

	return bResult;
}


bool	SaveStringToReg(HKEY hRootKey, LPCWSTR sKey, LPCWSTR sName, const std::wstring& rsValue, bool bDoWOW64 )
{
	HKEY	hKey	= NULL;
	DWORD	nFlags	= KEY_WRITE;

	if (bDoWOW64)
		nFlags |= KEY_WOW64_64KEY;

	if (RegCreateKeyEx(hRootKey, sKey, 0, NULL, REG_OPTION_NON_VOLATILE, nFlags, NULL, &hKey, NULL) != ERROR_SUCCESS)
		return false;

	bool bResult = (RegSetValueEx(hKey, sName, 0, REG_SZ, (BYTE*)rsValue.c_str(), 2*(DWORD)rsValue.length()) == ERROR_SUCCESS);

	RegCloseKey(hKey);

	return bResult;
}



typedef bool (__stdcall *EnumRegKeyCB)(LPCWSTR sKey, void* pData);

bool	EnumRegKey(HKEY hRootKey, LPCWSTR sKey, EnumRegKeyCB cd, void* pData, bool bDoWOW64)
{
	HKEY	hKey = NULL;

	if (!OpenKeyQuery(hKey, hRootKey, sKey, bDoWOW64, true))
		return false;

	DWORD cSubKeys=0; 

	RegQueryInfoKey(hKey,NULL,NULL,NULL,&cSubKeys,NULL,NULL,NULL,NULL,NULL,NULL,NULL);

	WCHAR    sChildKey[MAX_PATH];

	for (DWORD i=0; i<cSubKeys; i++) 
	{
		DWORD nChildKey = MAX_PATH;

		LONG nError = RegEnumKeyEx(hKey,i,sChildKey,&nChildKey,NULL,NULL,NULL,NULL);

        if (nError == ERROR_SUCCESS) 
			if (!cd(sChildKey, pData))
				break;
	}

	RegCloseKey(hKey);

	return true;
}
