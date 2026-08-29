#ifndef LUNAPASSPORT_UTIL_H
#define LUNAPASSPORT_UTIL_H

#include <windows.h>
#include <string>
#include <vector>

std::wstring Utf8ToWide(const std::string& text);
std::string WideToUtf8(const std::wstring& text);

bool IsRunningAsAdmin();
bool RelaunchAsAdmin();

bool IsValidIPv4(const std::string& ip);
bool IsValidDomain(const std::string& domain);

std::wstring GetSystemHostsPath();
std::wstring GetHostsBackupPath();

void AppendLog(HWND edit, const std::wstring& line);
std::wstring FormatWin32Error(DWORD error);

#endif
