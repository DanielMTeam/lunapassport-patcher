#include "app.h"
#include "cert.h"
#include "config.h"
#include "hosts.h"
#include "i18n.h"
#include "registry.h"
#include "util.h"

#include <commctrl.h>
#include <string>

namespace {

const wchar_t kWindowClass[] = L"LunaPassportPatcherWindow";
const int kMargin = 16;
const int kLabelHeight = 16;
const int kEditHeight = 24;
const int kButtonHeight = 30;
const int kButtonWidth = 118;
const int kButtonGap = 10;
const int kWindowWidth = 560;
const int kWindowHeight = 500;
const int kHeaderHeight = 56;
const int kGroupGap = 12;
const int kGroupCaption = 18;
const int kInnerPadX = 12;
const int kInnerPadTop = 10;
const int kInnerPadBottom = 8;

const COLORREF kColorHeaderBg = RGB(41, 98, 204);
const COLORREF kColorHeaderText = RGB(255, 255, 255);
const COLORREF kColorSubText = RGB(220, 230, 255);
const COLORREF kColorWindowBg = RGB(243, 246, 250);
const COLORREF kColorEditBg = RGB(255, 255, 255);
const COLORREF kColorText = RGB(32, 32, 32);

HWND gIpEdit = NULL;
HWND gDomainEdit = NULL;
HWND gPreviewEdit = NULL;
HWND gLogEdit = NULL;
HWND gLogGroup = NULL;
HWND gHeaderTitle = NULL;
HWND gHeaderSubtitle = NULL;

HFONT gFontUi = NULL;
HFONT gFontHeader = NULL;
HFONT gFontHeaderSub = NULL;
HBRUSH gBrushWindow = NULL;
HBRUSH gBrushHeader = NULL;
HBRUSH gBrushEdit = NULL;

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

void ApplyFont(HWND control, HFONT font) {
    if (control && font) {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
}

HFONT CreateAppFont(int height, bool bold) {
    return CreateFontW(
        height, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Tahoma");
}

void InitThemeResources() {
    gFontUi = CreateAppFont(-13, false);
    gFontHeader = CreateAppFont(-16, true);
    gFontHeaderSub = CreateAppFont(-12, false);
    gBrushWindow = CreateSolidBrush(kColorWindowBg);
    gBrushHeader = CreateSolidBrush(kColorHeaderBg);
    gBrushEdit = CreateSolidBrush(kColorEditBg);
}

void FreeThemeResources() {
    if (gFontUi) {
        DeleteObject(gFontUi);
        gFontUi = NULL;
    }
    if (gFontHeader) {
        DeleteObject(gFontHeader);
        gFontHeader = NULL;
    }
    if (gFontHeaderSub) {
        DeleteObject(gFontHeaderSub);
        gFontHeaderSub = NULL;
    }
    if (gBrushWindow) {
        DeleteObject(gBrushWindow);
        gBrushWindow = NULL;
    }
    if (gBrushHeader) {
        DeleteObject(gBrushHeader);
        gBrushHeader = NULL;
    }
    if (gBrushEdit) {
        DeleteObject(gBrushEdit);
        gBrushEdit = NULL;
    }
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
        message = Tr(STR_ERR_INVALID_IP);
        return false;
    }

    if (!IsValidDomain(domain)) {
        message = Tr(STR_ERR_INVALID_DOMAIN);
        return false;
    }

    if (!config.BuildFromInput(ip, domain)) {
        message = Tr(STR_ERR_BUILD_HOSTNAME);
        return false;
    }

    return true;
}

void ApplyPatch(HWND window) {
    PatchConfig config;
    std::wstring error;
    if (!ValidateInput(config, error)) {
        MessageBoxW(window, error.c_str(), Tr(STR_APP_TITLE), MB_ICONWARNING);
        return;
    }

    SetWindowTextW(gLogEdit, L"");
    std::wstring log;

    bool hostsOk = PatchHostsFile(config, log);
    bool registryOk = PatchPassportRegistry(config, log);
    bool certOk = ImportLetsEncryptRoot(log);

    SetWindowTextW(gLogEdit, log.c_str());

    if (hostsOk && registryOk && certOk) {
        std::wstring done = Tr(STR_MSG_DONE);
        done += Utf8ToWide(config.passportHost);
        done += L"/static/netpass/index.html";
        MessageBoxW(window, done.c_str(), Tr(STR_APP_TITLE), MB_ICONINFORMATION);
    } else {
        MessageBoxW(window, Tr(STR_MSG_ERRORS), Tr(STR_APP_TITLE), MB_ICONERROR);
    }
}

void VerifyInput(HWND window) {
    PatchConfig config;
    std::wstring error;
    if (!ValidateInput(config, error)) {
        MessageBoxW(window, error.c_str(), Tr(STR_APP_TITLE), MB_ICONWARNING);
        return;
    }

    UpdatePreview();

    std::wstring message = Tr(STR_MSG_INPUT_OK);
    message += Utf8ToWide(config.passportHost);
    message += L"\r\n";
    message += Utf8ToWide(config.memberservicesHost);
    MessageBoxW(window, message.c_str(), Tr(STR_APP_TITLE), MB_ICONINFORMATION);
}

HWND CreateLabel(HWND parent, const wchar_t* text, int x, int y, int width, int height) {
    HWND label = CreateWindowExW(0, L"STATIC", text,
                                 WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
                                 x, y, width, height, parent, NULL,
                                 GetModuleHandleW(NULL), NULL);
    ApplyFont(label, gFontUi);
    return label;
}

HWND CreateGroupBox(HWND parent, const wchar_t* text, int x, int y, int width, int height) {
    HWND group = CreateWindowExW(0, L"BUTTON", text,
                                 WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                 x, y, width, height, parent, NULL,
                                 GetModuleHandleW(NULL), NULL);
    ApplyFont(group, gFontUi);
    return group;
}

HWND CreateEdit(HWND parent, int id, int x, int y, int width, int height, bool readOnly) {
    DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
    if (readOnly) {
        style |= ES_READONLY | ES_MULTILINE;
    }

    HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", style,
                                x, y, width, height, parent,
                                reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                GetModuleHandleW(NULL), NULL);
    ApplyFont(edit, gFontUi);
    return edit;
}

HWND CreateButton(HWND parent, const wchar_t* text, int id, int x, int y, bool isDefault) {
    DWORD style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
    if (isDefault) {
        style |= BS_DEFPUSHBUTTON;
    }

    HWND button = CreateWindowExW(0, L"BUTTON", text, style,
                                  x, y, kButtonWidth, kButtonHeight, parent,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                  GetModuleHandleW(NULL), NULL);
    ApplyFont(button, gFontUi);
    return button;
}

void CreateHeader(HWND window) {
    RECT client;
    GetClientRect(window, &client);

    gHeaderTitle = CreateWindowExW(
        0, L"STATIC", Tr(STR_APP_TITLE),
        WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
        kMargin, 10, client.right - kMargin * 2, 22, window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_HEADER_TITLE)),
        GetModuleHandleW(NULL), NULL);
    ApplyFont(gHeaderTitle, gFontHeader);

    gHeaderSubtitle = CreateWindowExW(
        0, L"STATIC", Tr(STR_APP_SUBTITLE),
        WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
        kMargin, 32, client.right - kMargin * 2, 18, window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_HEADER_SUB)),
        GetModuleHandleW(NULL), NULL);
    ApplyFont(gHeaderSubtitle, gFontHeaderSub);
}

void LayoutLogPanel(HWND window) {
    if (!gLogGroup || !gLogEdit) {
        return;
    }

    RECT client;
    GetClientRect(window, &client);

    RECT groupRect;
    GetWindowRect(gLogGroup, &groupRect);
    MapWindowPoints(NULL, window, reinterpret_cast<POINT*>(&groupRect), 2);

    const int contentWidth = client.right - kMargin * 2;
    const int innerX = kMargin + kInnerPadX;
    const int innerWidth = contentWidth - kInnerPadX * 2;
    const int groupTop = groupRect.top;
    const int groupHeight = client.bottom - kMargin - groupTop;

    if (groupHeight < kGroupCaption + kInnerPadTop + kInnerPadBottom + 40) {
        return;
    }

    SetWindowPos(gLogGroup, NULL, kMargin, groupTop, contentWidth, groupHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    const int editTop = groupTop + kGroupCaption + kInnerPadTop;
    const int editHeight = groupHeight - kGroupCaption - kInnerPadTop - kInnerPadBottom;
    SetWindowPos(gLogEdit, NULL, innerX, editTop, innerWidth, editHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void CreateUi(HWND window) {
    RECT client;
    GetClientRect(window, &client);

    const int contentWidth = client.right - kMargin * 2;
    const int innerX = kMargin + kInnerPadX;
    const int innerWidth = contentWidth - kInnerPadX * 2;
    int y = kHeaderHeight + kGroupGap;

    const int connectionHeight =
        kGroupCaption + kInnerPadTop + (kLabelHeight + 4 + kEditHeight) * 2 + 8 + kInnerPadBottom;
    CreateGroupBox(window, Tr(STR_GROUP_CONNECTION), kMargin, y, contentWidth, connectionHeight);
    y += kGroupCaption + kInnerPadTop;

    CreateLabel(window, Tr(STR_LABEL_IP), innerX, y, innerWidth, kLabelHeight);
    y += kLabelHeight + 4;
    gIpEdit = CreateEdit(window, IDC_IP_EDIT, innerX, y, innerWidth, kEditHeight, false);
    y += kEditHeight + 8;

    CreateLabel(window, Tr(STR_LABEL_DOMAIN), innerX, y, innerWidth, kLabelHeight);
    y += kLabelHeight + 4;
    gDomainEdit = CreateEdit(window, IDC_DOMAIN_EDIT, innerX, y, innerWidth, kEditHeight, false);
    y += kEditHeight + kGroupGap;

    const int previewHeight = kGroupCaption + kInnerPadTop + 56 + kInnerPadBottom;
    CreateGroupBox(window, Tr(STR_GROUP_PREVIEW), kMargin, y, contentWidth, previewHeight);
    y += kGroupCaption + kInnerPadTop;
    gPreviewEdit = CreateEdit(window, IDC_PREVIEW_EDIT, innerX, y, innerWidth, 56, true);
    y += 56 + kGroupGap;

    CreateButton(window, Tr(STR_BTN_APPLY), IDC_APPLY_BTN, kMargin, y, true);
    CreateButton(window, Tr(STR_BTN_VERIFY), IDC_VERIFY_BTN,
                 kMargin + kButtonWidth + kButtonGap, y, false);
    CreateButton(window, Tr(STR_BTN_EXIT), IDC_EXIT_BTN,
                 kMargin + (kButtonWidth + kButtonGap) * 2, y, false);
    y += kButtonHeight + kGroupGap;

    const int logGroupTop = y;
    const int logGroupHeight = client.bottom - kMargin - logGroupTop;
    gLogGroup = CreateGroupBox(window, Tr(STR_GROUP_LOG), kMargin, logGroupTop, contentWidth,
                               logGroupHeight);

    const int logEditTop = logGroupTop + kGroupCaption + kInnerPadTop;
    const int logEditHeight =
        logGroupHeight - kGroupCaption - kInnerPadTop - kInnerPadBottom;
    gLogEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
            ES_READONLY | WS_VSCROLL,
        innerX, logEditTop, innerWidth, logEditHeight, window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LOG_EDIT)),
        GetModuleHandleW(NULL), NULL);
    ApplyFont(gLogEdit, gFontUi);

    SendMessageW(gIpEdit, EM_SETLIMITTEXT, 15, 0);
    SendMessageW(gDomainEdit, EM_SETLIMITTEXT, 253, 0);

    SetEditTextUtf8(gIpEdit, "1.1.1.1");
    SetEditTextUtf8(gDomainEdit, "lunastore.app");
    UpdatePreview();
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_ERASEBKGND: {
            HDC hdc = reinterpret_cast<HDC>(wparam);
            RECT client;
            GetClientRect(window, &client);
            FillRect(hdc, &client, gBrushWindow);
            RECT header = {0, 0, client.right, kHeaderHeight};
            FillRect(hdc, &header, gBrushHeader);
            return 1;
        }
        case WM_SIZE:
            LayoutLogPanel(window);
            return 0;
        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wparam);
            HWND control = reinterpret_cast<HWND>(lparam);
            if (control == gHeaderTitle) {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, kColorHeaderText);
                return reinterpret_cast<LRESULT>(gBrushHeader);
            }
            if (control == gHeaderSubtitle) {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, kColorSubText);
                return reinterpret_cast<LRESULT>(gBrushHeader);
            }
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, kColorText);
            return reinterpret_cast<LRESULT>(gBrushWindow);
        }
        case WM_CTLCOLOREDIT: {
            HDC hdc = reinterpret_cast<HDC>(wparam);
            SetBkColor(hdc, kColorEditBg);
            SetTextColor(hdc, kColorText);
            return reinterpret_cast<LRESULT>(gBrushEdit);
        }
        case WM_CTLCOLORBTN: {
            HDC hdc = reinterpret_cast<HDC>(wparam);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, kColorText);
            return reinterpret_cast<LRESULT>(gBrushWindow);
        }
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
            FreeThemeResources();
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(window, message, wparam, lparam);
}

}  // namespace

int RunApplication(HINSTANCE instance) {
    InitI18n();

    if (!IsRunningAsAdmin()) {
        int answer = MessageBoxW(NULL, Tr(STR_MSG_ADMIN_REQUIRED), Tr(STR_APP_TITLE),
                                 MB_YESNO | MB_ICONQUESTION);
        if (answer == IDYES && RelaunchAsAdmin()) {
            return 0;
        }
        return 1;
    }

    INITCOMMONCONTROLSEX controls;
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&controls);

    InitThemeResources();

    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = gBrushWindow;
    wc.lpszClassName = kWindowClass;

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, Tr(STR_ERR_REGISTER_CLASS), Tr(STR_APP_TITLE), MB_ICONERROR);
        FreeThemeResources();
        return 1;
    }

    RECT windowRect = {0, 0, kWindowWidth, kWindowHeight};
    AdjustWindowRectEx(&windowRect,
                       WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                       FALSE, WS_EX_APPWINDOW);

    HWND window = CreateWindowExW(
        WS_EX_APPWINDOW, kWindowClass, Tr(STR_APP_TITLE),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        NULL, NULL, instance, NULL);

    if (!window) {
        MessageBoxW(NULL, Tr(STR_ERR_CREATE_WINDOW), Tr(STR_APP_TITLE), MB_ICONERROR);
        FreeThemeResources();
        return 1;
    }

    CreateHeader(window);
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
