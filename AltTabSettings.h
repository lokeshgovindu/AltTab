#pragma once
#include <vector>
#include <unordered_set>
#include <string>
#include <wtypes.h>

using ProcessGroupsList = std::vector<std::unordered_set<std::wstring>>;

#define SETTINGS_INI_FILENAME   L"AltTabSettings.ini"

/*!
 * AltTab application settings
 * 
 * SimilarProcessGroups is a string, ProcessList are separated by | and processes are separated by /. 
 * Example:
 *    "notepad.exe/notepad++.exe|iexplore.exe/chrome.exe/firefox.exe|
 *     explorer.exe/xplorer2_lite.exe/xplorer2.exe/xplorer2_64.exe|
 *     cmd.exe/conemu.exe/conemu64.exe"
 */
struct AltTabSettings {
    std::wstring        FontName;               // Font Name
    int                 WidthPercentage;        // Window width in percentage of the actual screen width
    int                 HeightPercentage;       // Window height in percentage of the actual screen height
    int                 WindowWidth;            // Window width, will be calculated at runtime
    int                 WindowHeight;           // Window height, will be calculated at runtime
    COLORREF            FontColor;              // Font color
    COLORREF            BackgroundColor;        // Background color
    int                 Transparency;           // Window transparency
    std::wstring        SimilarProcessGroups;   // Similar process groups
    ProcessGroupsList   ProcessGroupsList;      // Process groups, will be constructed at runtime from SimilarProcessGroups
    std::wstring        CheckForUpdates;        // Check for updates
    bool                PromptTerminateAll;     // Ask before terminating all processes
    bool                DisableAltTab;          // Disable AltTab hotkeys

};

// ----------------------------------------------------------------------------
// Global declarations
// ----------------------------------------------------------------------------
extern AltTabSettings g_Settings;


INT_PTR CALLBACK ATSettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

int GetProcessGroupIndex(const std::wstring& processName);

/*!
 * Check if the processName is similar process of ProcessGroup[i]
 * 
 * \param[in]   index           Source process index
 * \param[in]   processName     Process name to check if it is available in ProcessGroup[i]
 * 
 * \returns true if the processName is of ProcessGroup[i] otherwise false.
 */
bool IsSimilarProcess(int index, const std::wstring& processName);

bool IsSimilarProcess(const std::wstring& processNameA, const std::wstring& processNameB);

std::wstring ATSettingsDirPath();

std::wstring ATSettingsFilePath();

void ATSettingsCreateDefault(const std::wstring& settingsFilePath);

void ATLoadSettings();

void ATSaveSettings();

