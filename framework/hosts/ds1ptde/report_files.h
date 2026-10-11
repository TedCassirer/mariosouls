#pragma once
#include <windows.h>
#include <string>
#include <utility>

namespace SCReportFiles {
struct Result {
    bool ok = false;
    std::wstring directory, review;
    DWORD error = 0;
};

inline bool writeNew(const std::wstring &path, const std::string &data, DWORD &error) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        error = GetLastError();
        return false;
    }
    DWORD written = 0;
    bool ok =
        WriteFile(h, data.data(), DWORD(data.size()), &written, nullptr) && written == data.size();
    if (!ok) {
        error = GetLastError();
        if (!error)
            error = ERROR_WRITE_FAULT;
    }
    CloseHandle(h);
    return ok;
}

// The worker owns its immutable snapshot. No game memory is read here.
inline Result save(const std::wstring &base, const std::string &json, const std::string &review) {
    Result result;
    if (json.empty() || json.size() > 512 * 1024 || review.empty() || review.size() > 64 * 1024) {
        result.error = ERROR_INVALID_DATA;
        return result;
    }
    SYSTEMTIME t{};
    GetSystemTime(&t);
    static volatile LONG sequence = 0;
    for (int attempt = 0; attempt < 64; attempt++) {
        wchar_t suffix[100];
        swprintf(suffix, 100, L"\\%04u%02u%02u-%02u%02u%02u-%lu-%ld", t.wYear, t.wMonth, t.wDay,
                 t.wHour, t.wMinute, t.wSecond, GetCurrentProcessId(),
                 InterlockedIncrement(&sequence));
        result.directory = base + suffix;
        if (CreateDirectoryW(result.directory.c_str(), nullptr))
            break;
        result.error = GetLastError();
        if (result.error != ERROR_ALREADY_EXISTS || attempt == 63)
            return result;
    }
    result.review = result.directory + L"\\review.html";
    result.ok = writeNew(result.directory + L"\\report.json", json, result.error) &&
                writeNew(result.review, review, result.error);
    if (result.ok)
        result.error = 0;
    return result;
}
} // namespace SCReportFiles
