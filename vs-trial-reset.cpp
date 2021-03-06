// vs-trial-reset.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <winreg.h>
#include <vector>
#include <sstream>
#include <ctime>
#include <iostream>

#pragma comment(lib, "Crypt32.lib")

#define LOG(strm) \
	std::wstringstream out; \
	out << strm << std::endl;\
	OutputDebugString(out.str().c_str());


// https://stackoverflow.com/questions/2344330/algorithm-to-add-or-subtract-days-from-a-date
// Adjust date by a number of days +/-
void DatePlusDays(struct tm* date, int days)
{
	const time_t ONE_DAY = 24 * 60 * 60;

	// Seconds since start of epoch
	time_t date_seconds = mktime(date) + (days * ONE_DAY);

	// Update caller's date
	// Use localtime because mktime converts to UTC so may change date
	*date = *localtime(&date_seconds);
}

void OnError(const std::wstring& msg)
{
	LOG(msg);
	std::wcout << msg << std::endl;
	system("pause");
	exit(1);
}

void doit(const wchar_t* path)
{
	DWORD tp = 0;
	std::vector<char> buf(1024);
	DWORD sz = buf.size();

	auto res = RegGetValue(HKEY_CLASSES_ROOT, path, L"", RRF_RT_REG_BINARY, &tp, buf.data(), &sz);
	if (res != ERROR_SUCCESS)
	{
		OnError(L"RegGetValue failed with " + std::to_wstring(GetLastError()));
	}

	DATA_BLOB blob1 = { sz, reinterpret_cast<BYTE*>(buf.data()) };
	DATA_BLOB blob2 = { };
	auto res2 = CryptUnprotectData(&blob1, nullptr, nullptr, nullptr, nullptr, 0, &blob2);
	if (res2 == FALSE)
	{
		OnError(L"CryptUnprotectData failed with " + std::to_wstring(GetLastError()));
	}

	short* year = reinterpret_cast<short*>(blob2.pbData + (blob2.cbData - 16));
	short* month = year + 1;
	short* day = year + 2;

	time_t now = time(nullptr);
	auto tmnow = localtime(&now);

	DatePlusDays(tmnow, 31);

	*year = tmnow->tm_year + 1900;
	*month = tmnow->tm_mon + 1;
	*day = tmnow->tm_mday;

	DATA_BLOB blob3 = { };
	res2 = CryptProtectData(&blob2, nullptr, nullptr, nullptr, nullptr, 0, &blob3);
	if (res2 == FALSE)
	{
		auto err = GetLastError();

		LocalFree(blob2.pbData);
		OnError(L"CryptProtectData failed with " + std::to_wstring(err));
	}

	LocalFree(blob2.pbData);

	HKEY hkey;
	res = RegOpenKeyEx(HKEY_CLASSES_ROOT, path, 0, KEY_SET_VALUE, &hkey);
	if (res != ERROR_SUCCESS)
	{
		auto err = GetLastError();

		LocalFree(blob3.pbData);
		OnError(L"RegOpenKeyEx failed with " + std::to_wstring(err));
	}

	res = RegSetValueEx(hkey, L"", 0, REG_BINARY, blob3.pbData, blob3.cbData);
	if (res != ERROR_SUCCESS)
	{
		auto err = GetLastError();

		LocalFree(blob3.pbData);
		RegCloseKey(hkey);
		OnError(L"RegSetValueEx failed with " + std::to_wstring(err));
	}

	LocalFree(blob3.pbData);
	RegCloseKey(hkey);
}

int main()
{
	std::wcout << L"Please close all Visual Studio instances before running this." << std::endl;

	doit(L"Licenses\\5C505A59-E312-4B89-9508-E162F8150517\\08878");
	doit(L"Licenses\\41717607-F34E-432C-A138-A3CFD7E25CDA\\09278");

	std::wcout << L"Trial successfully restarted." << std::endl;
	system("pause");

    return 0;
}
