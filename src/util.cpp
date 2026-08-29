#include "util.h"

#include <shellapi.h>
#include <sstream>

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return std::wstring();
    }

    int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
    if (size <= 0) {
        return std::wstring();
    }

    std::wstring result(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &result[0], size);
    return result;
}

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return std::string();
    }

    int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, NULL, 0, NULL, NULL);
    if (size <= 0) {
        return std::string();
    }

    std::string result(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &result[0], size, NULL, NULL);
    return result;
}

bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;

    if (!AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                  &adminGroup)) {
        return false;
    }

    CheckTokenMembership(NULL, adminGroup, &isAdmin);
    FreeSid(adminGroup);
    return isAdmin == TRUE;
}

bool RelaunchAsAdmin() {
    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(NULL, path, MAX_PATH) == 0) {
        return false;
    }

    HINSTANCE result = ShellExecuteW(NULL, L"runas", path, NULL, NULL, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(result) > 32;
}

static bool IsDigitChar(char c) {
    return c >= '0' && c <= '9';
}

bool IsValidIPv4(const std::string& ip) {
    if (ip.empty()) {
        return false;
    }

    int parts = 0;
    int value = 0;
    bool hasDigit = false;

    for (size_t i = 0; i < ip.size(); ++i) {
        char c = ip[i];
        if (c == '.') {
            if (!hasDigit || value > 255) {
                return false;
            }
            ++parts;
            value = 0;
            hasDigit = false;
        } else if (IsDigitChar(c)) {
            value = value * 10 + (c - '0');
            hasDigit = true;
        } else {
            return false;
        }
    }

    if (!hasDigit || value > 255) {
        return false;
    }

    return parts == 3;
}

bool IsValidDomain(const std::string& domain) {
    if (domain.size() < 3 || domain.size() > 253) {
        return false;
    }

    bool hasDot = false;
    for (size_t i = 0; i < domain.size(); ++i) {
        char c = domain[i];
        if (c == '.') {
            hasDot = true;
            continue;
        }
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-')) {
            return false;
        }
    }

    return hasDot;
}

std::wstring GetSystemHostsPath() {
    wchar_t systemRoot[MAX_PATH];
    if (GetEnvironmentVariableW(L"SystemRoot", systemRoot, MAX_PATH) == 0) {
        wcscpy(systemRoot, L"C:\\Windows");
    }

    std::wstring path = systemRoot;
    path += L"\\system32\\drivers\\etc\\hosts";
    return path;
}

std::wstring GetHostsBackupPath() {
    return GetSystemHostsPath() + L".lunapassport.bak";
}

void AppendLog(HWND edit, const std::wstring& line) {
    if (!edit) {
        return;
    }

    int length = GetWindowTextLengthW(edit);
    SendMessageW(edit, EM_SETSEL, length, length);
    std::wstring text = line + L"\r\n";
    SendMessageW(edit, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text.c_str()));
}

std::wstring FormatWin32Error(DWORD error) {
    wchar_t* buffer = NULL;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS;

    DWORD length = FormatMessageW(flags, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  reinterpret_cast<LPWSTR>(&buffer), 0, NULL);
    if (length == 0 || buffer == NULL) {
        std::wstringstream ss;
        ss << L"error " << error;
        return ss.str();
    }

    std::wstring message(buffer, length);
    LocalFree(buffer);

    while (!message.empty() && (message[message.size() - 1] == L'\r' ||
                                message[message.size() - 1] == L'\n')) {
        message.resize(message.size() - 1);
    }

    return message;
}
