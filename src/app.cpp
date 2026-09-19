#include "app.h"
#include "cert.h"
#include "config.h"
#include "hosts.h"
#include "i18n.h"
#include "registry.h"
#include "resource.h"
#include "util.h"

#include <commctrl.h>
#include <string>

namespace {

const wchar_t kWindowClass[] = L"LunaPassportPatcherWindow";
const int kMargin = 10;
const int kLabelColWidth = 110;
const int kLabelGap = 6;
const int kEditHeight = 22;
const int kRowGap = 6;
const int kButtonHeight = 26;
const int kButtonWidth = 100;
const int kButtonGap = 8;
const int kWindowWidth = 420;
const int kWindowHeight = 288;
const int kHeaderHeight = 48;
const int kGroupGap = 8;
const int kGroupCaption = 16;
const int kInnerPadX = 8;
const int kInnerPadTop = 6;
const int kInnerPadBottom = 6;
const int kLogEditHeight = 70;

const COLORREF kColorHeaderBg = RGB(41, 98, 204);
const COLORREF kColorHeaderText = RGB(255, 255, 255);
const COLORREF kColorSubText = RGB(220, 230, 255);
const COLORREF kColorWindowBg = RGB(243, 246, 250);
const COLORREF kColorEditBg = RGB(255, 255, 255);
const COLORREF kColorText = RGB(32, 32, 32);

HWND gIpEdit = NULL;
HWND gDomainEdit = NULL;
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
HBITMAP gHeaderBitmap = NULL;

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
    gFontHeader = CreateAppFont(-14, true);
    gFontHeaderSub = CreateAppFont(-11, false);
    gBrushWindow = CreateSolidBrush(kColorWindowBg);
    gBrushHeader = CreateSolidBrush(kColorHeaderBg);
    gBrushEdit = CreateSolidBrush(kColorEditBg);
    gHeaderBitmap = static_cast<HBITMAP>(LoadImageW(
        GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_HEADER_BG), IMAGE_BITMAP, 0, 0,
        LR_DEFAULTCOLOR));
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
    if (gHeaderBitmap) {
        DeleteObject(gHeaderBitmap);
        gHeaderBitmap = NULL;
    }
}

void PaintHeaderBackground(HDC hdc, int width) {
    RECT header = {0, 0, width, kHeaderHeight};
    if (!gHeaderBitmap) {
        FillRect(hdc, &header, gBrushHeader);
        return;
    }

    HDC mem = CreateCompatibleDC(hdc);
    if (!mem) {
        FillRect(hdc, &header, gBrushHeader);
        return;
    }

    HGDIOBJ old = SelectObject(mem, gHeaderBitmap);
    BITMAP bm;
    ZeroMemory(&bm, sizeof(bm));
    GetObject(gHeaderBitmap, sizeof(bm), &bm);
    if (bm.bmWidth > 0 && bm.bmHeight > 0) {
        StretchBlt(hdc, 0, 0, width, kHeaderHeight, mem, 0, 0, bm.bmWidth, bm.bmHeight,
                   SRCCOPY);
    } else {
        FillRect(hdc, &header, gBrushHeader);
    }
    SelectObject(mem, old);
    DeleteDC(mem);
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

HWND CreateEdit(HWND parent, int id, int x, int y, int width, int height) {
    HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
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
        kMargin, 8, client.right - kMargin * 2, 16, window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_HEADER_TITLE)),
        GetModuleHandleW(NULL), NULL);
    ApplyFont(gHeaderTitle, gFontHeader);

    gHeaderSubtitle = CreateWindowExW(
        0, L"STATIC", Tr(STR_APP_SUBTITLE),
        WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
        kMargin, 26, client.right - kMargin * 2, 14, window,
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
    const int groupHeight =
        kGroupCaption + kInnerPadTop + kLogEditHeight + kInnerPadBottom;

    SetWindowPos(gLogGroup, NULL, kMargin, groupTop, contentWidth, groupHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    const int editTop = groupTop + kGroupCaption + kInnerPadTop;
    SetWindowPos(gLogEdit, NULL, innerX, editTop, innerWidth, kLogEditHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void CreateUi(HWND window) {
    RECT client;
    GetClientRect(window, &client);

    const int contentWidth = client.right - kMargin * 2;
    const int editX = kMargin + kLabelColWidth + kLabelGap;
    const int editWidth = client.right - kMargin - editX;
    int y = kHeaderHeight + kGroupGap;

    const int labelY = y + (kEditHeight - 16) / 2;
    CreateLabel(window, Tr(STR_LABEL_IP), kMargin, labelY, kLabelColWidth, 16);
    gIpEdit = CreateEdit(window, IDC_IP_EDIT, editX, y, editWidth, kEditHeight);
    y += kEditHeight + kRowGap;

    const int domainLabelY = y + (kEditHeight - 16) / 2;
    CreateLabel(window, Tr(STR_LABEL_DOMAIN), kMargin, domainLabelY, kLabelColWidth, 16);
    gDomainEdit = CreateEdit(window, IDC_DOMAIN_EDIT, editX, y, editWidth, kEditHeight);
    y += kEditHeight + kGroupGap;

    CreateButton(window, Tr(STR_BTN_APPLY), IDC_APPLY_BTN, kMargin, y, true);
    CreateButton(window, Tr(STR_BTN_VERIFY), IDC_VERIFY_BTN,
                 kMargin + kButtonWidth + kButtonGap, y, false);
    CreateButton(window, Tr(STR_BTN_EXIT), IDC_EXIT_BTN,
                 kMargin + (kButtonWidth + kButtonGap) * 2, y, false);
    y += kButtonHeight + kGroupGap;

    const int logGroupHeight =
        kGroupCaption + kInnerPadTop + kLogEditHeight + kInnerPadBottom;
    gLogGroup = CreateGroupBox(window, Tr(STR_GROUP_LOG), kMargin, y, contentWidth,
                               logGroupHeight);

    const int logEditTop = y + kGroupCaption + kInnerPadTop;
    const int logInnerX = kMargin + kInnerPadX;
    const int logInnerWidth = contentWidth - kInnerPadX * 2;
    gLogEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
            ES_READONLY | WS_VSCROLL,
        logInnerX, logEditTop, logInnerWidth, kLogEditHeight, window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LOG_EDIT)),
        GetModuleHandleW(NULL), NULL);
    ApplyFont(gLogEdit, gFontUi);

    SendMessageW(gIpEdit, EM_SETLIMITTEXT, 15, 0);
    SendMessageW(gDomainEdit, EM_SETLIMITTEXT, 253, 0);

    SetEditTextUtf8(gIpEdit, "37.139.63.83");
    SetEditTextUtf8(gDomainEdit, "lunastore.app");
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_ERASEBKGND: {
            HDC hdc = reinterpret_cast<HDC>(wparam);
            RECT client;
            GetClientRect(window, &client);
            FillRect(hdc, &client, gBrushWindow);
            PaintHeaderBackground(hdc, client.right);
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
                return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
            }
            if (control == gHeaderSubtitle) {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, kColorSubText);
                return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
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
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = gBrushWindow;
    wc.lpszClassName = kWindowClass;
    wc.hIconSm = static_cast<HICON>(LoadImageW(
        instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));

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
