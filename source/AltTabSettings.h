#pragma once
#include <vector>
#include <unordered_set>
#include <string>
#include <wtypes.h>
#include <wingdi.h>

using ProcessGroupsList    = std::vector<std::unordered_set<std::wstring>>;
using ProcessExclusionList = std::vector<std::wstring>;

#define SETTINGS_INI_FILENAME                L"AltTabSettings.ini"

#define DEFAULT_FONT_NAME                    L"Lucida Handwriting"
#define DEFAULT_FONT_SIZE                    11
#define DEFAULT_FONT_STYLE                   L"normal"               // normal, italic, bold and bold italic
#define DEFAULT_FONT_COLOR                   RGB(0xFF, 0xFF, 0xFF)
#define DEFAULT_BG_COLOR                     RGB(0x00, 0x00, 0x00)
#define DEFAULT_WIDTH                        45
#define DEFAULT_HEIGHT                       45
#define DEFAULT_FUZZYMATCHPERCENT            60
#define DEFAULT_TRANSPARENCY                 222
#define DEFAULT_SIMILARPROCESSGROUPS         L"notepad.exe/notepad++.exe|iexplore.exe/chrome.exe/firefox.exe|explorer.exe/xplorer2_lite.exe/xplorer2.exe/xplorer2_64.exe|cmd.exe/conemu.exe/conemu64.exe"
#define DEFAULT_CHECKFORUPDATES              L"Startup"
#define DEFAULT_PROMPTTERMINATEALL           true
#define DEFAULT_SHOW_COL_HEADER              false
#define DEFAULT_SHOW_COL_PROCESSNAME         false
#define DEFAULT_PROCESS_EXCLUSIONS_ENABLED   false
#define DEFAULT_PROCESS_EXCLUSIONS           L""

/*!
 * AltTab application settings
 * 
 * SimilarProcessGroups is a string, ProcessList are separated by | and processes are separated by /. 
 * Example:
 *    "notepad.exe/notepad++.exe|iexplore.exe/chrome.exe/firefox.exe|
 *     explorer.exe/xplorer2_lite.exe/xplorer2.exe/xplorer2_64.exe|
 *     cmd.exe/conemu.exe/conemu64.exe"
 * 
 * ProcessExclusions is a string, Process names are separated by /.
 * Example:
 *    "outlook.exe/teams.exe"
 */
struct AltTabSettings {
    std::wstring           FontName;                  // Font Name
    int                    FontSize;                  // Font Size
    std::wstring           FontStyle;                 // Font Style
    COLORREF               FontColor;                 // Font color
    COLORREF               BackgroundColor;           // Background color
    int                    WidthPercentage;           // Window width in percentage of the actual screen width
    int                    HeightPercentage;          // Window height in percentage of the actual screen height
    int                    WindowWidth;               // Window width, will be calculated at runtime
    int                    WindowHeight;              // Window height, will be calculated at runtime
    int                    FuzzyMatchPercent;         // Fuzzy match percent
    int                    Transparency;              // Window transparency
    std::wstring           SimilarProcessGroups;      // Similar process groups
    ProcessGroupsList      ProcessGroupsList;         // Process groups, will be constructed at runtime from SimilarProcessGroups
    std::wstring           CheckForUpdates;           // Check for updates
    bool                   PromptTerminateAll;        // Ask before terminating all processes
    bool                   DisableAltTab;             // Disable AltTab hotkeys
    bool                   ShowColHeader;             // Show column header
    bool                   ShowColProcessName;        // Show column - Process Name
    bool                   ProcessExclusionsEnabled;  // Process exclusions enabled
    std::wstring           ProcessExclusions;         // Process exclusions string which are separated by /
    ProcessExclusionList   ProcessExclusionList;      // Process exclusions list, will be constructed at runtime from ProcessExclusions

    AltTabSettings();
};

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

std::wstring ATSettingsFilePath(bool overwrite = false);

void ATSettingsToFile(const std::wstring& settingsFilePath);

/*!
 * Load settings from AltTabSettings.ini file to application
 */
void ATLoadSettings();

/*!
 * Save application settings to AltTabSettings.ini file
 */
void ATSaveSettings();

/*!
 * Save modified settings to AltTabSettings.ini file
 */
void ATApplySettings(HWND hDlg);

/*!
 * Check if settings are modified
 * 
 * \returns true if settings are modified otherwise false.
 */
bool AreSettingsModified(HWND hDlg);

