#include "registry.h"
#include "i18n.h"
#include "util.h"

#include <sstream>

static bool SetStringValue(HKEY key, const wchar_t* name, const std::string& value) {
    std::wstring wide = Utf8ToWide(value);
    LONG result = RegSetValueExW(key, name, 0, REG_SZ,
                                 reinterpret_cast<const BYTE*>(wide.c_str()),
                                 static_cast<DWORD>((wide.size() + 1) * sizeof(wchar_t)));
    return result == ERROR_SUCCESS;
}

static bool SetDwordValue(HKEY key, const wchar_t* name, DWORD value) {
    LONG result = RegSetValueExW(key, name, 0, REG_DWORD,
                                 reinterpret_cast<const BYTE*>(&value),
                                 sizeof(DWORD));
    return result == ERROR_SUCCESS;
}

bool PatchPassportRegistry(const PatchConfig& config, std::wstring& log) {
    const wchar_t* subKey =
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Passport";

    HKEY key = NULL;
    LONG result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, NULL, 0,
                                  KEY_SET_VALUE, NULL, &key, NULL);
    if (result != ERROR_SUCCESS) {
        log += Tr(STR_LOG_REG_OPEN_ERR) + FormatWin32Error(result) + L"\r\n";
        return false;
    }

    bool ok = true;
    ok = SetDwordValue(key, L"ConfigVersion", 0x11) && ok;
    ok = SetStringValue(key, L"LoginServerRealm", "Passport.Net") && ok;
    ok = SetStringValue(key, L"LoginServerUrl",
                        "https://" + config.passportHost + "/login2.asp") && ok;
    ok = SetStringValue(key, L"RegistrationUrl",
                        "https://" + config.passportHost + "/defaultwiz.asp") && ok;
    ok = SetStringValue(key, L"GeneralRedir",
                        "https://" + config.passportHost + "/redir.asp") && ok;
    ok = SetStringValue(key, L"Privacy",
                        "https://" + config.passportHost + "/consumer/privacypolicy.asp") &&
         ok;
    ok = SetStringValue(key, L"Properties",
                        "https://" + config.memberservicesHost +
                            "/ppsecure/MSRV_EditProfile.asp") &&
         ok;
    ok = SetStringValue(key, L"Help",
                        "https://" + config.memberservicesHost + "/UI/MSRV_UI_Help.asp") &&
         ok;

    RegCloseKey(key);

    if (!ok) {
        log += Tr(STR_LOG_REG_WRITE_ERR);
        return false;
    }

    log += Tr(STR_LOG_REG_UPDATED);
    return true;
}
