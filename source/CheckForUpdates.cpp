#include "framework.h"
#include "CheckForUpdates.h"
#include <string>
#include "Logger.h"
#include "Utils.h"

#include <wininet.h>
#include "version.h"
#include <WinUser.h>
#include "resource.h"
#include <shellapi.h>
#include <format>
#include <chrono>
#include <ctime>
#include <fstream>
#include "AltTabSettings.h"
#include <filesystem>

#pragma comment(lib, "wininet.lib")

struct CFU_Info {
    std::string CurrentVersion;
    std::string UpdateVersion;
    std::string Changes;
} g_UpdatesInfo;


INT_PTR CALLBACK ATCheckForUpdatesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
std::chrono::system_clock::time_point ReadLastCheckForUpdatesTS();
void        WriteCheckForUpdatesTS(const std::chrono::system_clock::time_point& timestamp);

// Function to compare version strings
int CompareVersions(const std::string& updateVersion, const std::string& currentVersion) {
    // Implement your version comparison logic here
    // Return 1 if v1 > v2, -1 if v1 < v2, and 0 if v1 == v2
    return updateVersion.compare(currentVersion);
}

std::wstring GetLastErrorEx() {
    // Get the last error code
    DWORD errorCode = GetLastError();

    // Get the error message
    LPVOID errorMessage;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr,
        errorCode,
        0,
        reinterpret_cast<LPWSTR>(&errorMessage),
        0,
        nullptr);
    if (!errorMessage)
        return L"";
    std::wstring ret = reinterpret_cast<LPCWSTR>(errorMessage);
    AT_LOG_ERROR("  Error Code   : %d", errorCode);
    AT_LOG_ERROR("  Error Message: %s", WStrToUTF8(ret).c_str());
    return ret;
}

// Function to check for updates from a URL using Windows API
void CheckForUpdates(bool quiteMode) {
    AT_LOG_TRACE;
    //{
    //    DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_CHECK_FOR_UPDATES), nullptr, ATCheckForUpdatesDlgProc);
    //    return;
    //}
    // Initialize WinINet
    HINTERNET hInternet = InternetOpen(L"Sample WinINet", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        AT_LOG_ERROR("Error initializing WinINet");
        GetLastErrorEx();
        return;
    }

    // Open a URL
    HINTERNET hConnect = InternetOpenUrl(hInternet, AT_UPDATE_FILE_URL, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        AT_LOG_ERROR("Error opening URL with WinINet");
        GetLastErrorEx();
        InternetCloseHandle(hInternet);
        return;
    }

    // Read and print the response content
    char buffer[1024] = { 0 };
    DWORD bytesRead;
    std::string contents;
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        contents.append(buffer);
    }

    // Close handles
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    // Compare versions
    std::string currentVersion = AT_VERSION_TEXT;
    auto remainingLines        = Split(contents, "\n");
    std::string updateVersion  = Trim(remainingLines[0]);
    std::string installerName  = Trim(remainingLines[1]);
    std::string latestChanges;
    for (int i = 2; i < remainingLines.size(); i++) {
        latestChanges += remainingLines[i] + "\n";
    }

    if (CompareVersions(updateVersion, currentVersion) > 0) {
        AT_LOG_INFO("Update available!");
        AT_LOG_INFO("Latest Version: %s", updateVersion.c_str());
        g_UpdatesInfo.CurrentVersion = currentVersion;
        g_UpdatesInfo.UpdateVersion  = updateVersion;
        g_UpdatesInfo.Changes        = latestChanges;
        DialogBoxW(g_hInstance, MAKEINTRESOURCE(IDD_CHECK_FOR_UPDATES), nullptr, ATCheckForUpdatesDlgProc);
    } else {
        std::string info = std::format(
            "You are using the latest version of {}.\n\nCurrentVersion:\t{}\nUpdateVersion:\t{}",
            AT_PRODUCT_NAMEA,
            currentVersion,
            updateVersion);
        if (quiteMode) {
            AT_LOG_INFO(info.c_str());
        } else {
            MessageBoxA(nullptr, info.c_str(), AT_PRODUCT_NAMEA, MB_OK | MB_ICONINFORMATION);
        }
    }
}

INT_PTR CALLBACK ATCheckForUpdatesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG: {
        HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ALTTAB));
        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

        // Center the dialog on the screen
        int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        RECT dlgRect;
        GetWindowRect(hDlg, &dlgRect);

        int dlgWidth  = dlgRect.right - dlgRect.left;
        int dlgHeight = dlgRect.bottom - dlgRect.top;

        int posX = (screenWidth  - dlgWidth ) / 2;
        int posY = (screenHeight - dlgHeight) / 2;

        SetWindowPos(hDlg, HWND_TOP, posX, posY, 0, 0, SWP_NOSIZE);

        // Set the dialog as an app window, otherwise not displayed in task bar
        SetWindowLong(hDlg, GWL_EXSTYLE, GetWindowLong(hDlg, GWL_EXSTYLE) | WS_EX_APPWINDOW);

        HFONT hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Lucida Console");
 
        HWND hEditBox1 = GetDlgItem(hDlg, IDC_EDIT_CFU_CHANGES);
        SendMessage(hEditBox1, WM_SETFONT, (WPARAM)hFont, TRUE);

        SetDlgItemTextA(hDlg, IDC_STATIC_CFU_CURRENT_VERSION, g_UpdatesInfo.CurrentVersion.c_str());
        SetDlgItemTextA(hDlg, IDC_STATIC_CFU_UPDATE_VERSION , g_UpdatesInfo.UpdateVersion.c_str());
        SetDlgItemTextA(hDlg, IDC_EDIT_CFU_CHANGES          , g_UpdatesInfo.Changes.c_str());
    }
    break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            EndDialog(hDlg, LOWORD(wParam));
            ShellExecuteW(nullptr, L"open", AT_PRODUCT_LATEST_URL, nullptr, nullptr, SW_SHOWNORMAL);
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;

    case WM_DESTROY:
        //  Clean up
        HWND hEditBox = GetDlgItem(hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS);
        HFONT hFont = (HFONT)SendMessage(hEditBox, WM_GETFONT, 0, 0);
        if (hFont) {
            DeleteObject(hFont);
        }
        DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, ICON_SMALL, 0));
        DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, ICON_BIG, 0));
        break;
    }
    return (INT_PTR)FALSE;
}

std::chrono::system_clock::time_point ReadLastCheckForUpdatesTS() {
    std::filesystem::path filePath = ATSettingsDirPath();
    filePath.append(CHECK_FOR_UPDATES_FILENAME);
    std::ifstream file(filePath.string());

    if (file.is_open()) {
        long long timestamp;
        file >> timestamp;
        file.close();
        return std::chrono::system_clock::from_time_t(timestamp);

    }

    return {};
}

void WriteCheckForUpdatesTS(const std::chrono::system_clock::time_point& timestamp) {
    std::filesystem::path filePath = ATSettingsDirPath();
    filePath.append(CHECK_FOR_UPDATES_FILENAME);

    std::ofstream file(filePath.string());
    file << std::chrono::system_clock::to_time_t(timestamp);
    file.close();
}
