#include "i18n.h"

#include <windows.h>

namespace {

enum AppLanguage {
    LANG_EN = 0,
    LANG_RU = 1
};

AppLanguage gLanguage = LANG_EN;

const wchar_t* kStringsEn[STR_COUNT] = {
    L"LunaPassport Patcher",
    L"Configure LunaPassport (.NET Passport) endpoints on Windows XP",
    L"Server IP:",
    L"Base domain:",
    L"Log",
    L"Apply",
    L"Verify",
    L"Exit",
    L"Invalid IPv4 address.",
    L"Invalid domain. Example: lunastore.app",
    L"Could not build hostnames from input.",
    L"Done.\r\nRestart Internet Explorer and open:\r\nhttps://",
    L"Errors occurred. See the log.",
    L"Input is valid.\r\n\r\n",
    L"Administrator rights are required.\r\nRestart as administrator?",
    L"Could not register window class.",
    L"Could not create window.",
    L"[WARN] hosts backup failed: ",
    L"[OK] hosts backup: ",
    L"[ERR] cannot write temp hosts file\r\n",
    L"[ERR] cannot replace hosts: ",
    L"[OK] hosts updated\r\n",
    L"[ERR] registry open failed: ",
    L"[ERR] registry write failed\r\n",
    L"[OK] Passport registry updated\r\n",
    L"[ERR] embedded CA resource missing\r\n",
    L"[OK] Let's Encrypt root already installed\r\n",
    L"[ERR] cannot open ROOT store\r\n",
    L"[ERR] CA import failed: ",
    L"[OK] Let's Encrypt root imported\r\n"
};

const wchar_t* kStringsRu[STR_COUNT] = {
    L"LunaPassport Patcher",
    L"Настройка LunaPassport (.NET Passport) на Windows XP",
    L"IP сервера:",
    L"Базовый домен:",
    L"Журнал",
    L"Применить",
    L"Проверить",
    L"Выход",
    L"Неверный IPv4-адрес.",
    L"Неверный домен. Пример: lunastore.app",
    L"Не удалось построить hostname из ввода.",
    L"Готово.\r\nПерезапустите Internet Explorer и откройте:\r\nhttps://",
    L"Есть ошибки. Смотрите журнал.",
    L"Ввод корректен.\r\n\r\n",
    L"Нужны права администратора.\r\nПерезапустить от имени администратора?",
    L"Не удалось зарегистрировать класс окна.",
    L"Не удалось создать окно.",
    L"[WARN] резервная копия hosts не создана: ",
    L"[OK] резервная копия hosts: ",
    L"[ERR] не удалось записать временный hosts\r\n",
    L"[ERR] не удалось заменить hosts: ",
    L"[OK] hosts обновлён\r\n",
    L"[ERR] не удалось открыть реестр: ",
    L"[ERR] не удалось записать реестр\r\n",
    L"[OK] реестр Passport обновлён\r\n",
    L"[ERR] встроенный CA не найден\r\n",
    L"[OK] корневой Let's Encrypt уже установлен\r\n",
    L"[ERR] не удалось открыть хранилище ROOT\r\n",
    L"[ERR] не удалось импортировать CA: ",
    L"[OK] корневой Let's Encrypt импортирован\r\n"
};

}  // namespace

void InitI18n() {
    LANGID lang = PRIMARYLANGID(GetUserDefaultUILanguage());
    if (lang == LANG_RUSSIAN || lang == LANG_UKRAINIAN || lang == LANG_BELARUSIAN) {
        gLanguage = LANG_RU;
    } else {
        gLanguage = LANG_EN;
    }
}

const wchar_t* Tr(StringId id) {
    if (id < 0 || id >= STR_COUNT) {
        return L"";
    }
    return gLanguage == LANG_RU ? kStringsRu[id] : kStringsEn[id];
}
