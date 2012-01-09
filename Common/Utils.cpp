
#include "Utils.h"


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


const char*	GetUserName()
{
	static char sBuffer[256] = {0};
	DWORD nLength = 255;

	if (sBuffer[0] != 0)
		return sBuffer;

	::GetUserNameA(sBuffer, &nLength);

	return sBuffer;
}