#include "framework.h"
#include "Utils.h"
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include "Logger.h"

bool EnableConsoleWindow() {
    if (!AllocConsole())
        return false; // Could throw here
    FILE* stream = nullptr;
    freopen_s(&stream, "CONOUT$", "w", stdout);
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
