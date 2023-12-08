#pragma once
#include <vector>
#include <unordered_set>
#include <string>
#include <wtypes.h>

using ProcessGroupsList = std::vector<std::unordered_set<std::wstring>>;

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
    int                 WindowTransparency;     // Window transparency
    std::wstring        SimilarProcessGroups;   // Similar process groups
    ProcessGroupsList   ProcessGroupsList;      // Process groups, will be constructed at runtime from SimilarProcessGroups
};

extern AltTabSettings g_Settings;
