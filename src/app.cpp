#include "app.h"
#include "cert.h"
#include "config.h"
#include "hosts.h"
#include "registry.h"
#include "util.h"

#include <commctrl.h>
#include <string>

namespace {

const wchar_t kWindowClass[] = L"LunaPassportPatcherWindow";
const int kMargin = 12;
const int kLabelHeight = 14;
const int kEditHeight = 22;
const int kButtonHeight = 26;
const int kButtonWidth = 110;
const int kWindowWidth = 520;
const int kWindowHeight = 430;

HWND gIpEdit = NULL;
HWND gDomainEdit = NULL;
HWND gPreviewEdit = NULL;
HWND gLogEdit = NULL;

std::string GetEditTextUtf8(HWND edit) {
    int length = GetWindowTextLengthW(edit);
    if (length <= 0) {
        return std::string();
    }

    std::wstring wide(length + 1, L'\0');
    GetWindowTextW(edit, &wide[0], length + 1);
    wide.resize(length);
    return WideToUtf8(wide);
}

void SetEditTextUtf8(HWND edit, const std::string& text) {
    SetWindowTextW(edit, Utf8ToWide(text).c_str());
}

void UpdatePreview() {
    PatchConfig config;
    std::string ip = GetEditTextUtf8(gIpEdit);
    std::string domain = GetEditTextUtf8(gDomainEdit);
    if (!config.BuildFromInput(ip, domain)) {
        SetWindowTextW(gPreviewEdit, L"");
        return;
    }

    SetWindowTextW(gPreviewEdit, config.PreviewText().c_str());
}

bool ValidateInput(PatchConfig& config, std::wstring& message) {
    std::string ip = GetEditTextUtf8(gIpEdit);
    std::string domain = GetEditTextUtf8(gDomainEdit);

    if (!IsValidIPv4(ip)) {
        message = L"Неверный IPv4-адрес.";
        return false;
    }

    if (!IsValidDomain(domain)) {
        message = L"Неверный домен. Пример: pidoras.top";
        return false;
    }

    if (!config.BuildFromInput(ip, domain)) {
        message = L"Не удалось построить hostname из ввода.";
        return false;
    }

    return true;
}

void ApplyPatch(HWND window) {
    PatchConfig config;
    std::wstring error;
    if (!ValidateInput(config, error)) {
        MessageBoxW(window, error.c_str(), L"LunaPassport Patcher", MB_ICONWARNING);
        return;
    }

    SetWindowTextW(gLogEdit, L"");
    std::wstring log;

    bool hostsOk = PatchHostsFile(config, log);
    bool registryOk = PatchPassportRegistry(config, log);
    bool certOk = ImportLetsEncryptRoot(log);

    SetWindowTextW(gLogEdit, log.c_str());

    if (hostsOk && registryOk && certOk) {
        std::wstring done = L"Готово.\r\nПерезапустите Internet Explorer и откройте:\r\nhttps://";
        done += Utf8ToWide(config.passportHost);
        done += L"/static/netpass/index.html";
        MessageBoxW(window, done.c_str(), L"LunaPassport Patcher", MB_ICONINFORMATION);
    } else {
        MessageBoxW(window, L"Есть ошибки. Смотрите лог.", L"LunaPassport Patcher",
                    MB_ICONERROR);
    }
}

void VerifyInput(HWND window) {
    PatchConfig config;
    std::wstring error;
    if (!ValidateInput(config, error)) {
        MessageBoxW(window, error.c_str(), L"LunaPassport Patcher", MB_ICONWARNING);
        return;
    }

    UpdatePreview();

    std::wstring message = L"Ввод корректен.\r\n\r\n";
    message += Utf8ToWide(config.passportHost);
    message += L"\r\n";
    message += Utf8ToWide(config.memberservicesHost);
    MessageBoxW(window, message.c_str(), L"LunaPassport Patcher", MB_ICONINFORMATION);
}

HWND CreateLabel(HWND parent, const wchar_t* text, int x, int y, int width) {
    return CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE,
                           x, y, width, kLabelHeight, parent, NULL,
                           GetModuleHandleW(NULL), NULL);
}

HWND CreateEdit(HWND parent, int id, int x, int y, int width, bool readOnly) {
    DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
    if (readOnly) {
        style |= ES_READONLY | ES_MULTILINE;
    }

    return CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", style,
                           x, y, width, readOnly ? 54 : kEditHeight, parent,
                           reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                           GetModuleHandleW(NULL), NULL);
}

HWND CreateButton(HWND parent, const wchar_t* text, int id, int x, int y) {
    return CreateWindowExW(0, L"BUTTON", text,
                           WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                           x, y, kButtonWidth, kButtonHeight, parent,
                           reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                           GetModuleHandleW(NULL), NULL);
}

void CreateUi(HWND window) {
    const int contentWidth = kWindowWidth - kMargin * 2;
    int y = kMargin;

    CreateLabel(window, L"IP сервера:", kMargin, y, contentWidth);
    y += kLabelHeight + 4;
    gIpEdit = CreateEdit(window, IDC_IP_EDIT, kMargin, y, contentWidth, false);
    y += kEditHeight + 10;

    CreateLabel(window, L"Базовый домен (пример: pidoras.top):", kMargin, y, contentWidth);
    y += kLabelHeight + 4;
    gDomainEdit = CreateEdit(window, IDC_DOMAIN_EDIT, kMargin, y, contentWidth, false);
    y += kEditHeight + 10;

    CreateLabel(window, L"Предпросмотр:", kMargin, y, contentWidth);
    y += kLabelHeight + 4;
    gPreviewEdit = CreateEdit(window, IDC_PREVIEW_EDIT, kMargin, y, contentWidth, true);
    y += 54 + 10;

    CreateButton(window, L"Применить", IDC_APPLY_BTN, kMargin, y);
    CreateButton(window, L"Проверить", IDC_VERIFY_BTN, kMargin + kButtonWidth + 8, y);
    CreateButton(window, L"Выход", IDC_EXIT_BTN, kMargin + (kButtonWidth + 8) * 2, y);
    y += kButtonHeight + 10;

    CreateLabel(window, L"Log:", kMargin, y, contentWidth);
    y += kLabelHeight + 4;
    gLogEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER |
                                      ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY |
                                      WS_VSCROLL,
        kMargin, y, contentWidth, kWindowHeight - y - kMargin, window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LOG_EDIT)),
        GetModuleHandleW(NULL), NULL);

    SendMessageW(gIpEdit, EM_SETLIMITTEXT, 15, 0);
    SendMessageW(gDomainEdit, EM_SETLIMITTEXT, 253, 0);

    SetEditTextUtf8(gIpEdit, "185.183.181.51");
    SetEditTextUtf8(gDomainEdit, "pidoras.top");
    UpdatePreview();
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_COMMAND:
            switch (LOWORD(wparam)) {
                case IDC_APPLY_BTN:
                    ApplyPatch(window);
                    return 0;
                case IDC_VERIFY_BTN:
                    VerifyInput(window);
                    return 0;
                case IDC_EXIT_BTN:
                    DestroyWindow(window);
                    return 0;
                case IDC_IP_EDIT:
                case IDC_DOMAIN_EDIT:
                    if (HIWORD(wparam) == EN_CHANGE) {
                        UpdatePreview();
                    }
                    return 0;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(window, message, wparam, lparam);
}

}  // namespace

int RunApplication(HINSTANCE instance) {
    if (!IsRunningAsAdmin()) {
        int answer = MessageBoxW(
            NULL,
            L"Нужны права администратора.\r\nПерезапустить от имени администратора?",
            L"LunaPassport Patcher", MB_YESNO | MB_ICONQUESTION);
        if (answer == IDYES && RelaunchAsAdmin()) {
            return 0;
        }
        return 1;
    }

    INITCOMMONCONTROLSEX controls;
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&controls);

    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kWindowClass;

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"Не удалось зарегистрировать класс окна.", L"LunaPassport Patcher",
                    MB_ICONERROR);
        return 1;
    }

    HWND window = CreateWindowExW(
        0, kWindowClass, L"LunaPassport Patcher",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, kWindowWidth, kWindowHeight, NULL, NULL, instance, NULL);

    if (!window) {
        MessageBoxW(NULL, L"Не удалось создать окно.", L"LunaPassport Patcher", MB_ICONERROR);
        return 1;
    }

    CreateUi(window);
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
