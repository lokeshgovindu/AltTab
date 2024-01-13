#include "framework.h"
#include "Utils.h"
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string>
#include <algorithm>
#include "../fuzzywuzzy.h"

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

// Convert wstring to UTF-8 string
std::string WStrToUTF8(const std::wstring& wstr) {
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), nullptr, 0, nullptr, nullptr);
    std::string utf8str(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), &utf8str[0], sizeNeeded, nullptr, nullptr);
    return utf8str;
}

// Convert UTF-8 string to wstring
std::wstring UTF8ToWStr(const std::string& utf8str) {
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), static_cast<int>(utf8str.length()), nullptr, 0);
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), static_cast<int>(utf8str.length()), &wstr[0], sizeNeeded);
    return wstr;
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

bool InStr(const std::wstring& str, const std::wstring& substr) {
    return std::search(
               str.begin(),
               str.end(),
               substr.begin(),
               substr.end(),
               [](wchar_t c1, wchar_t c2) { return std::toupper(c1) == std::toupper(c2); })
           != str.end();
}

#define BUFFER_SIZE 1024

std::string ToLower(const char* s) {
    std::string ret;
    for (int i = 0; s[i] != '\0'; ++i) {
        if (isupper(s[i])) {
            ret += tolower(s[i]);
        } else {
            ret += s[i];
        }
    }
    return ret;
}

std::string ToNarrow(const wchar_t* szBufW) {
    char szBuf[BUFFER_SIZE];
    size_t count;
    errno_t err;

    err = wcstombs_s(&count, szBuf, (size_t)BUFFER_SIZE, szBufW, (size_t)BUFFER_SIZE);
    if (err != 0) {
        throw std::exception("Failed to convert to a multi byte character string.");
    }
    return ::ToLower(szBuf);
}

double GetRatioA(const char* s1, const char* s2) {
    //printf("s1 = [%s], s2 = [%s]\n", s1, s2);
    return ::ratio(ToLower(s1), ToLower(s2));
}

double GetPartialRatioA(const char* s1, const char* s2) {
    //printf("s1 = [%s], s2 = [%s]\n", s1, s2);
    return ::partial_ratio(ToLower(s1), ToLower(s2));
}

double GetRatioW(const wchar_t* s1, const wchar_t* s2) {
    try {
        //wprintf(L"s1 = [%s], s2 = [%s]\n", s1, s2);
        return ::ratio(ToNarrow(s1), ToNarrow(s2));
    } catch (...) {
        return 0.0;
    }
}

struct ScopedTimer {
    ScopedTimer() {
        QueryPerformanceCounter(&m_tStartTime);
    }

    ~ScopedTimer() {
        QueryPerformanceCounter(&m_tStopTime);
        QueryPerformanceFrequency(&m_tFrequency);
        m_Elapsed = (double)(m_tStopTime.QuadPart - m_tStartTime.QuadPart) / (double)m_tFrequency.QuadPart;

        printf_s("Elapsed : %f\n", m_Elapsed);
    }

private:
    LARGE_INTEGER m_tStartTime;
    LARGE_INTEGER m_tStopTime;
    LARGE_INTEGER m_tFrequency;
    std::string m_Started;
    std::string m_Ended;
    double m_Elapsed;
    std::string m_Name;
};

double GetPartialRatioW(const wchar_t* s1, const wchar_t* s2) {
    //ScopedTimer st;
    try {
        return partial_ratio(ToNarrow(s1), ToNarrow(s2));
    } catch (...) {
        return 0.0;
    }
}
