#include "framework.h"
#include "Utils.h"
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include "Logger.h"
#include <string>
#include <algorithm>

bool EnableConsoleWindow() {
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        if (!AllocConsole())
            return false; // Could throw here
    }
    
    FILE* stdinNew = nullptr;
    freopen_s(&stdinNew,  "CONIN$",  "r+", stdin);
    FILE* stdoutNew = nullptr;
    freopen_s(&stdoutNew, "CONOUT$", "w+", stdout);
    FILE* stderrNew = nullptr;
    freopen_s(&stderrNew, "CONOUT$", "w+", stderr);

    return true;
}

std::vector<std::wstring> Split(const std::wstring& s, const std::wstring& seps) {
    std::vector<std::wstring> ret;
    for (size_t p = 0, q; p != std::wstring::npos; p = q) {
        p = s.find_first_not_of(seps, p);
        if (p == std::wstring::npos)
            break;
        q = s.find_first_of(seps, p);
        ret.push_back(s.substr(p, q - p));
    }
    return ret;
}

bool EqualsIgnoreCase(const std::string& str1, const std::string& t) {
    return str1.size() == t.size() && std::equal(str1.begin(), str1.end(), t.begin(), [](auto a, auto b) {
        return std::tolower(a) == std::tolower(b);
    });
}

bool EqualsIgnoreCase(const std::wstring& str1, const std::wstring& t) {
    return str1.size() == t.size() && std::equal(str1.begin(), str1.end(), t.begin(), [](auto a, auto b) {
        return std::tolower(a) == std::tolower(b);
    });
}

std::wstring ToLower(const std::wstring& s) {
    std::wstring result = s;
    std::transform(result.begin(), result.end(), result.begin(), towlower);
    return result;
}

std::wstring ToUpper(const std::wstring& s) {
    std::wstring result = s;
    std::transform(result.begin(), result.end(), result.begin(), towupper);
    return result;
}

std::string GetWindowTitleExA(HWND hWnd) {
    const int bufferSize = GetWindowTextLengthA(hWnd) + 1;
    char* windowTitle = new char[bufferSize];
    GetWindowTextA(hWnd, windowTitle, bufferSize);
    std::string ret = windowTitle;
    delete[] windowTitle;
    return ret;
}

std::wstring GetWindowTitleExW(HWND hWnd) {
    const int bufferSize = GetWindowTextLengthW(hWnd) + 1;
    wchar_t* windowTitle = new wchar_t[bufferSize];
    GetWindowTextW(hWnd, windowTitle, bufferSize);
    std::wstring ret = windowTitle;
    delete[] windowTitle;
    return ret;
}
