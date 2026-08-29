#include "hosts.h"
#include "i18n.h"
#include "util.h"

#include <sstream>
#include <string>
#include <vector>

static const char kBeginMarker[] = "# BEGIN LunaPassport";
static const char kEndMarker[] = "# END LunaPassport";

static bool ReadWholeFile(const std::wstring& path, std::string& content) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        content.clear();
        return false;
    }

    DWORD size = GetFileSize(file, NULL);
    if (size == INVALID_FILE_SIZE) {
        CloseHandle(file);
        return false;
    }

    content.assign(size, '\0');
    DWORD read = 0;
    BOOL ok = ReadFile(file, &content[0], size, &read, NULL);
    CloseHandle(file);

    if (!ok) {
        content.clear();
        return false;
    }

    content.resize(read);
    return true;
}

static bool WriteWholeFile(const std::wstring& path, const std::string& content) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    BOOL ok = WriteFile(file, content.data(), static_cast<DWORD>(content.size()), &written, NULL);
    CloseHandle(file);
    return ok && written == content.size();
}

static void RemoveMarkedBlock(std::string& content) {
    while (true) {
        size_t begin = content.find(kBeginMarker);
        if (begin == std::string::npos) {
            break;
        }

        size_t end = content.find(kEndMarker, begin);
        if (end == std::string::npos) {
            content.erase(begin);
            break;
        }

        end += strlen(kEndMarker);
        while (end < content.size() && (content[end] == '\r' || content[end] == '\n')) {
            ++end;
        }

        content.erase(begin, end - begin);
    }
}

static void TrimTrailingNewlines(std::string& content) {
    while (!content.empty() && (content[content.size() - 1] == '\n' ||
                                content[content.size() - 1] == '\r')) {
        content.erase(content.size() - 1);
    }
}

bool PatchHostsFile(const PatchConfig& config, std::wstring& log) {
    const std::wstring hostsPath = GetSystemHostsPath();
    const std::wstring backupPath = GetHostsBackupPath();

    std::string content;
    if (!ReadWholeFile(hostsPath, content)) {
        content.clear();
    }

    if (!CopyFileW(hostsPath.c_str(), backupPath.c_str(), FALSE)) {
        DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND) {
            log += Tr(STR_LOG_HOSTS_BACKUP_WARN) + FormatWin32Error(error) + L"\r\n";
        }
    } else {
        log += Tr(STR_LOG_HOSTS_BACKUP_OK) + backupPath + L"\r\n";
    }

    RemoveMarkedBlock(content);
    TrimTrailingNewlines(content);

    if (!content.empty()) {
        content += "\r\n";
    }

    content += kBeginMarker;
    content += "\r\n";
    content += config.ip;
    content += " ";
    content += config.passportHost;
    content += " ";
    content += config.memberservicesHost;
    content += " register.passport.com\r\n";
    content += kEndMarker;
    content += "\r\n";

    const std::wstring tempPath = hostsPath + L".lunapassport.tmp";
    if (!WriteWholeFile(tempPath, content)) {
        log += Tr(STR_LOG_HOSTS_WRITE_ERR);
        return false;
    }

    if (!MoveFileExW(tempPath.c_str(), hostsPath.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DWORD error = GetLastError();
        DeleteFileW(tempPath.c_str());
        log += Tr(STR_LOG_HOSTS_REPLACE_ERR) + FormatWin32Error(error) + L"\r\n";
        return false;
    }

    log += Tr(STR_LOG_HOSTS_UPDATED);
    return true;
}
