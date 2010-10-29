
#include "Utils.h"
#include "RegUtils.h"


class IsInWOW64Cache
{
public:
	IsInWOW64Cache()
	{
		typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

		m_bAnswer = false;

		LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(L"kernel32"),"IsWow64Process");

		if (fnIsWow64Process != NULL)
		{
			BOOL bIs64 = FALSE;

			if (fnIsWow64Process(GetCurrentProcess(), &bIs64) && bIs64)
				m_bAnswer = true;
		}
	}

	bool	WellWhat()	{ return m_bAnswer; }
private:
	bool	m_bAnswer;
};


bool		IsInWOW64()
{
	static IsInWOW64Cache cache;

	return cache.WellWhat();
}


bool	LoadDWORDFromReg(HKEY hRootKey, LPCWSTR sKey, LPCWSTR sName, DWORD& rnValue, bool bDoWOW64 )
{
	HKEY	hKey	= NULL;
	DWORD	nFlags	= KEY_QUERY_VALUE;

	if (bDoWOW64)
		nFlags |= KEY_WOW64_64KEY;

	if (RegOpenKeyEx(hRootKey, sKey, 0, nFlags, &hKey) != ERROR_SUCCESS)
		return false;

	int		nNumber		= 0;
	DWORD	nBufSize	= sizeof(nNumber);
	bool	bResult		= (RegQueryValueEx(hKey, sName, 0, NULL, (BYTE*)&nNumber, &nBufSize) == ERROR_SUCCESS);

	RegCloseKey(hKey);

	rnValue = nNumber;

	return bResult;
}


bool	StoreDWORDFromReg(HKEY hRootKey, LPCWSTR sKey, LPCWSTR sName, DWORD nValue, bool bDoWOW64 )
{
	HKEY	hKey	= NULL;
	DWORD	nFlags	= KEY_WRITE;

	if (bDoWOW64)
		nFlags |= KEY_WOW64_64KEY;

	if (RegCreateKeyEx(hRootKey, sKey, 0, NULL, REG_OPTION_NON_VOLATILE, nFlags, NULL, &hKey, NULL) != ERROR_SUCCESS)
		return false;

	bool bResult = (RegSetValueEx(hRootKey, sName, 0, REG_DWORD, (BYTE*)&nValue, sizeof(nValue)) == ERROR_SUCCESS);

	RegCloseKey(hKey);

	return bResult;
}


bool	LoadStringFromReg(HKEY hRootKey, LPCWSTR sKey, LPCWSTR sName, std::wstring& rsValue, bool bDoWOW64 )
{
	HKEY	hKey	= NULL;
	DWORD	nFlags	= KEY_QUERY_VALUE;

	if (bDoWOW64)
		nFlags |= KEY_WOW64_64KEY;

	if (RegOpenKeyEx(hRootKey, sKey, 0, nFlags, &hKey) != ERROR_SUCCESS)
		return false;

	bool	bResult		= false;
	DWORD	nBufSize	= 0;

	RegQueryValueEx(hKey, sName, 0, NULL, NULL, &nBufSize);

	if (nBufSize > 0)
	{
		rsValue.assign(nBufSize/2,L' '); // /2 because it's wchar not char

		RegQueryValueEx(hKey, sName, 0, NULL, (LPBYTE)rsValue.c_str(), &nBufSize);

		//Remove any extra terminations

		while(rsValue[rsValue.length()-1] == '\0')
			rsValue.resize(rsValue.length()-1);

		bResult = true;
	}
	
	RegCloseKey(hKey);

	return bResult;
}


bool	StoreStringFromReg(HKEY hRootKey, LPCWSTR sKey, LPCWSTR sName, const std::wstring& rsValue, bool bDoWOW64 )
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

