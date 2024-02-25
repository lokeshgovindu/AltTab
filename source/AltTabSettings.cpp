#include "AltTabSettings.h"
#include "Resource.h"
#include <windowsx.h>
#include <CommCtrl.h>
#include "Utils.h"
#include "Logger.h"
#include <ShlObj_core.h>
#include "version.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include "GlobalData.h"
#include "AltTabWindow.h"
#include "Tooltips.h"
#include <regex>

#define WM_SETTEXTCOLOR (WM_USER + 1)

AltTabSettings    g_Settings;
AltTabWindowData  g_AltBacktickWndInfo;
HWND              g_hToolTip           = nullptr;


#define ADD_TOOLTIP(id, text)                                                    \
    toolInfo.hwnd = GetDlgItem(hDlg, id);                                        \
    toolInfo.lpszText = (LPWSTR)text;                                            \
    GetClientRect(toolInfo.hwnd, &toolInfo.rect);                                \
    SendMessage(g_hToolTip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));


void ATSettingsInitDialog(HWND hDlg, const AltTabSettings& settings);
void ATReadSettingsFromUI(HWND hDlg, AltTabSettings& settings);
void AddTooltips         (HWND hDlg);
void ATLogSettings       (const AltTabSettings& settings);

/*!
 * \brief Constructor
 */
AltTabSettings::AltTabSettings() {
    Reset();
}

/*!
 * \brief Reset settings to default values.
 */
void AltTabSettings::Reset() {
    FontName                 = DEFAULT_FONT_NAME;
    FontSize                 = DEFAULT_FONT_SIZE;
    FontStyle                = DEFAULT_FONT_STYLE;
    FontColor                = DEFAULT_FONT_COLOR;
    BackgroundColor          = DEFAULT_BG_COLOR;
    WidthPercentage          = DEFAULT_WIDTH;
    HeightPercentage         = DEFAULT_HEIGHT;
    FuzzyMatchPercent        = DEFAULT_FUZZYMATCHPERCENT;
    Transparency             = DEFAULT_TRANSPARENCY;
    SimilarProcessGroups     = DEFAULT_SIMILARPROCESSGROUPS;
    CheckForUpdatesOpt       = DEFAULT_CHECKFORUPDATES;
    PromptTerminateAll       = DEFAULT_PROMPTTERMINATEALL;
    DisableAltTab            = false;
    ShowColHeader            = DEFAULT_SHOW_COL_HEADER;
    ShowColProcessName       = DEFAULT_SHOW_COL_PROCESSNAME;
    ProcessExclusionsEnabled = DEFAULT_PROCESS_EXCLUSIONS_ENABLED;
    ProcessExclusions        = DEFAULT_PROCESS_EXCLUSIONS;

    // Clear the previous ProcessGroupsList
    g_Settings.ProcessGroupsList.clear();

    auto vs = Split(g_Settings.SimilarProcessGroups, L"|");
    for (auto& item : vs) {
        auto processes = Split(item, L"/");
        for (auto& processName : processes)
            processName = ToLower(processName);
        g_Settings.ProcessGroupsList.emplace_back(processes.begin(), processes.end());
    }

    // Process ProcessExclusions
    // Always split and convert to lower case, then it is easy while checking
    g_Settings.ProcessExclusionList.clear();
    g_Settings.ProcessExclusionList = Split(ToLower(g_Settings.ProcessExclusions), L"/");

    // Initialize additional settings
    g_AltBacktickWndInfo.hWnd   = nullptr;
    g_AltBacktickWndInfo.hOwner = nullptr;
}

int AltTabSettings::GetCheckForUpdatesIndex() const {
    auto it = std::find(CheckForUpdatesOptions.begin(), CheckForUpdatesOptions.end(), this->CheckForUpdatesOpt);
    if (it == CheckForUpdatesOptions.end()) {
        return 0;
    }
    return (int)std::distance(CheckForUpdatesOptions.begin(), it);
}

/**
 * \brief Add tooltips to AltTab settings dialog controls.
 * 
 * \param hDlg
 */
void AddTooltips(HWND hDlg) {
    g_hToolTip = CreateWindowEx(
        0,
        TOOLTIPS_CLASS,
        nullptr,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        hDlg,
        nullptr,
        g_hInstance,
        nullptr);

    TOOLINFO toolInfo = { 0 };
    toolInfo.cbSize   = sizeof(toolInfo);
    toolInfo.hwnd     = hDlg;
    toolInfo.uFlags   = TTF_SUBCLASS;

    // Enable multiple lines
    SendMessage(g_hToolTip, TTM_SETMAXTIPWIDTH, 0, MAXINT);

    // TODO: Not working
    SendMessage(g_hToolTip, TTM_SETTIPBKCOLOR, RGB(255, 255, 0), 0);

    ADD_TOOLTIP(IDC_EDIT_SETTINGS_FILEPATH       , TT_SETTINGS_FILEPATH       );
    ADD_TOOLTIP(IDC_EDIT_SIMILAR_PROCESS_GROUPS  , TT_SIMILAR_PROCESS_GROUPS  );
    ADD_TOOLTIP(IDC_EDIT_FUZZY_MATCH_PERCENT     , TT_FUZZY_MATCH_PERCENT     );
    ADD_TOOLTIP(IDC_STATIC_FUZZY_MATCH_PERCENT   , TT_FUZZY_MATCH_PERCENT     ); // TODO: Not working for static controls
    ADD_TOOLTIP(IDC_EDIT_WINDOW_TRANSPARENCY     , TT_WINDOW_TRANSPARENCY     );
    ADD_TOOLTIP(IDC_EDIT_WINDOW_WIDTH_PERCENTAGE , TT_WINDOW_WIDTH_PERCENT    );
    ADD_TOOLTIP(IDC_EDIT_WINDOW_HEIGHT_PERCENTAGE, TT_WINDOW_HEIGHT_PERCENT   );
    ADD_TOOLTIP(IDC_CHECK_PROMPT_TERMINATE_ALL   , TT_PROMPT_TERMINATE_ALL    );
    ADD_TOOLTIP(IDC_CHECK_SHOW_COL_HEADER        , TT_SHOW_COLUMN_HEADER      );
    ADD_TOOLTIP(IDC_CHECK_SHOW_COL_PROCESSNAME   , TT_SHOW_COLUMN_PROCESS_NAME);
    ADD_TOOLTIP(IDC_CHECK_FOR_UPDATES            , TT_CHECK_FOR_UPDATES       );
    ADD_TOOLTIP(IDC_CHECK_PROCESS_EXCLUSIONS     , TT_CHECK_PROCESS_EXCLUSIONS);
    ADD_TOOLTIP(IDC_EDIT_PROCESS_EXCLUSIONS      , TT_EDIT_PROCESS_EXCLUSIONS );
    ADD_TOOLTIP(IDC_BUTTON_APPLY                 , TT_APPLY_SETTINGS          );
    ADD_TOOLTIP(IDOK                             , TT_OK_SETTINGS             );
    ADD_TOOLTIP(IDCANCEL                         , TT_CANCEL_SETTINGS         );
    ADD_TOOLTIP(IDC_BUTTON_RESET                 , TT_RESET_SETTINGS          );
}

// ----------------------------------------------------------------------------
// Settings dialog procedure
// ----------------------------------------------------------------------------
INT_PTR CALLBACK ATSettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG: {
        // Store the settings window handle in global variable
        g_hSetingsWnd      = hDlg;

        HICON hIcon        = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ALTTAB));

        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

        ATSettingsInitDialog(hDlg, g_Settings);

        AddTooltips(hDlg);
    }
    return (INT_PTR)TRUE;

    case WM_CTLCOLOREDIT: {
        HDC  hdcEdit = (HDC)wParam;
        HWND hEdit   = (HWND)lParam;
        int  id      = GetDlgCtrlID(hEdit);
        if (id == IDC_EDIT_SIMILAR_PROCESS_GROUPS || id == IDC_EDIT_PROCESS_EXCLUSIONS) {
            SetTextColor(hdcEdit, RGB(0, 0, 255));   // Blue text color
        }
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    }
    break;

    //case WM_CTLCOLORSTATIC:
    //{
    //    HDC  hdcEdit = (HDC)wParam;
    //    HWND hEdit   = (HWND)lParam;
    //    SetTextColor(hdcEdit, RGB(255, 0, 255));
    //    return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    //}
    //break;

    //case WM_CTLCOLORSTATIC: {
    //    // Handle WM_CTLCOLORSTATIC to customize the text color of the group box
    //    HDC  hdcStatic = (HDC)wParam;
    //    HWND hStatic   = (HWND)lParam;

    //    // Check if the control is a group box (BS_GROUPBOX style)
    //    if (GetWindowLong(hStatic, GWL_STYLE) & BS_GROUPBOX) {
    //        AT_LOG_INFO("BS_GROUPBOX: Group Box Found");
    //        // Set the text color for the group box
    //        SetTextColor(hdcStatic, RGB(0, 0, 255));
    //        SetBkColor(hdcStatic, TRANSPARENT);

    //        // Return a handle to the brush for the background
    //        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    //    }
    //} break;

    case WM_DRAWITEM: {
        AT_LOG_INFO("WM_DRAWITEM: Draw Item");
    } break;

    case WM_COMMAND: {
        bool settingsModified = false;
        if (g_hSetingsWnd) {
            settingsModified = AreSettingsModified(hDlg);
            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_APPLY), settingsModified);
        }

        if (LOWORD(wParam) == IDC_BUTTON_APPLY) {
            AT_LOG_INFO("IDC_BUTTON_APPLY: Apply Settings");
            if (settingsModified) { ATApplySettings(hDlg); }
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDOK)
        {
            if (settingsModified) { ATApplySettings(hDlg); }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDCANCEL)
        {
            AT_LOG_INFO("IDCANEL: Cancel Pressed!");
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDC_BUTTON_RESET)
        {
            AT_LOG_INFO("IDC_BUTTON_RESET Pressed!");
            int result = MessageBoxW(
                hDlg,
                L"Are you sure you want to reset settings to defaults?",
                AT_PRODUCT_NAMEW L": Reset Settings",
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
            if (result == IDYES) {
                g_Settings.Reset();
                ATSettingsInitDialog(hDlg, g_Settings);
            }
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDC_CHECK_PROCESS_EXCLUSIONS) {
            AT_LOG_INFO("IDC_CHECK_PROCESS_EXCLUSIONS Clicked!");
            bool isChecked = IsDlgButtonChecked(hDlg, IDC_CHECK_PROCESS_EXCLUSIONS) == BST_CHECKED;
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROCESS_EXCLUSIONS), isChecked);
            return (INT_PTR)TRUE;
        }
    } break;

    //case WM_NOTIFY: {
    //    AT_LOG_INFO("WM_NOTIFY: Notify");
    //    LPNMHDR pnmh = (LPNMHDR)lParam;
    //    if (pnmh->code == TTN_NEEDTEXT) {
    //        LPNMTTDISPINFO pDispInfo = (LPNMTTDISPINFO)lParam;
    //        if (pDispInfo->hdr.idFrom == IDC_STATIC_FUZZY_MATCH_PERCENT) {
    //            // Set the tooltip text for the static control
    //            pDispInfo->lpszText = (LPWSTR)L"Tooltip for Static Control";
    //        }
    //    }
    //} break;

    case WM_DESTROY: {
        //  Clean up
        HWND  hEditBox = GetDlgItem(hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS);
        HFONT hFont    = (HFONT)SendMessage(hEditBox, WM_GETFONT, 0, 0);
        if (hFont) {
            DeleteObject(hFont);
        }
        DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, ICON_SMALL, 0));
        DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, ICON_BIG, 0));

        g_hSetingsWnd = nullptr;
    }
    break;

    }
    return (INT_PTR)FALSE;
}

int GetProcessGroupIndex(const std::wstring& processName) {
    const std::wstring& processNameL = ToLower(processName);
    for (size_t ind = 0; ind < g_Settings.ProcessGroupsList.size(); ++ind) {
        if (g_Settings.ProcessGroupsList[ind].contains(processNameL)) {
            return (int)ind;
        }
    }
    return -1;
}

bool IsSimilarProcess(int index, const std::wstring& processName) {
    if (index == -1)
        return false;
    return g_Settings.ProcessGroupsList[index].contains(ToLower(processName));
}

bool IsSimilarProcess(const std::wstring& processNameA, const std::wstring& processNameB) {
    if (EqualsIgnoreCase(processNameA, processNameB))
        return true;

    int index = GetProcessGroupIndex(processNameA);
    if (index == -1)
        return false;

    return g_Settings.ProcessGroupsList[index].contains(ToLower(processNameB));
}

/*!
 * \brief AltTab settings directory path
 * 
 * \return AltTab settings directory path
 */
std::wstring ATSettingsDirPath() {
    // Get the path to the Roaming AppData folder
    wchar_t szPath[MAX_PATH] = { 0 };
    SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, szPath);
    std::filesystem::path settingsDirPath = szPath;
    settingsDirPath.append(AT_PRODUCT_NAMEW);
    if (!std::filesystem::exists(settingsDirPath)) {
        if (!std::filesystem::create_directory(settingsDirPath)) {
            throw std::exception("Failed to create AltTab directory in APPDATA");
        }
    }
    return settingsDirPath.wstring();
}

// ----------------------------------------------------------------------------
// AltTab settings file path
// ----------------------------------------------------------------------------
std::wstring ATSettingsFilePath(bool overwrite) {
    AT_LOG_INFO("overwrite = %d", overwrite);
    std::filesystem::path settingsFilePath = ATSettingsDirPath();
    settingsFilePath.append(SETTINGS_INI_FILENAME);
    if (!std::filesystem::exists(settingsFilePath) || overwrite) {
        std::ofstream fs(settingsFilePath);
        if (!fs.is_open()) {
            throw std::exception("Failed to create AltTab.ini file in APPDATA/AltTab");
        }
        fs << "; -----------------------------------------------------------------------------" << std::endl;
        fs << "; Configuration/settings file for AltTab." << std::endl;
        fs << "; Notes:" << std::endl;
        fs << ";   1. Do NOT edit manually if you are not familiar with settings." << std::endl;
        fs << ";   2. Color Format is RGB(0xAA, 0xBB, 0xCC) => 0xAABBCC, in hex format." << std::endl;
        fs << ";      0xAA : Red component" << std::endl;
        fs << ";      0xBB : Green component" << std::endl;
        fs << ";      0xCC : Blue component" << std::endl;
        fs << ";   3. FontStyle: normal / italic / bold / bold italic" << std::endl;
        fs << ";   4. Please delete this file to create a new settings file when AltTab opens." << std::endl;
        fs << "; -----------------------------------------------------------------------------" << std::endl;
        fs.close();
        ATSettingsToFile(settingsFilePath.wstring());
    }
    return settingsFilePath.wstring();
}

template<typename T>
void WriteSetting(const std::wstring& iniFile, LPCTSTR section, LPCTSTR keyName, const T& value) {
    WritePrivateProfileStringW(section, keyName, std::to_wstring(value).c_str(), iniFile.c_str());
}

template<>
void WriteSetting(const std::wstring& iniFile, LPCTSTR section, LPCTSTR keyName, const std::wstring& value) {
    WritePrivateProfileStringW(section, keyName, value.c_str(), iniFile.c_str());
}

template<typename T, typename DefaultType>
void ReadSetting(const std::wstring& iniFile, LPCTSTR section, LPCTSTR keyName, DefaultType defaultValue, T& value) {
    value = GetPrivateProfileIntW(section, keyName, defaultValue, iniFile.c_str());
}

template<>
void ReadSetting(const std::wstring& iniFile, LPCTSTR section, LPCTSTR keyName, LPCTSTR defaultValue, std::wstring& value) {
    const int bufferSize = 4096; // Initial buffer size
    wchar_t buffer[bufferSize];  // Buffer to store the retrieved string
    GetPrivateProfileStringW(section, keyName, defaultValue, buffer, bufferSize, iniFile.c_str());
    value = buffer;
}

/*!
 * \brief Write the current settings the given file path
 * 
 * \param iniFile    AltTab settings file path
 */
void ATSettingsToFile(const std::wstring& iniFile) {
    WriteSetting(iniFile, L"ListView"         , L"FontName"              , g_Settings.FontName                );
    WriteSetting(iniFile, L"ListView"         , L"FontSize"              , g_Settings.FontSize                );
    WriteSetting(iniFile, L"ListView"         , L"FontStyle"             , g_Settings.FontStyle               );
    WriteSetting(iniFile, L"ListView"         , L"FontColor"             , g_Settings.FontColor               );
    WriteSetting(iniFile, L"ListView"         , L"BackgroundColor"       , g_Settings.BackgroundColor         );
    WriteSetting(iniFile, L"Backtick"         , L"SimilarProcessGroups"  , g_Settings.SimilarProcessGroups    );
    WriteSetting(iniFile, L"General"          , L"PromptTerminateAll"    , g_Settings.PromptTerminateAll      );
    WriteSetting(iniFile, L"General"          , L"FuzzyMatchPercent"     , g_Settings.FuzzyMatchPercent       );
    WriteSetting(iniFile, L"General"          , L"WindowTransparency"    , g_Settings.Transparency            );
    WriteSetting(iniFile, L"General"          , L"WindowWidthPercentage" , g_Settings.WidthPercentage         );
    WriteSetting(iniFile, L"General"          , L"WindowHeightPercentage", g_Settings.HeightPercentage        );
    WriteSetting(iniFile, L"General"          , L"ShowColHeader"         , g_Settings.ShowColHeader           );
    WriteSetting(iniFile, L"General"          , L"ShowColProcessName"    , g_Settings.ShowColProcessName      );
    WriteSetting(iniFile, L"General"          , L"CheckForUpdates"       , g_Settings.CheckForUpdatesOpt      );
    WriteSetting(iniFile, L"ProcessExclusions", L"Enabled"               , g_Settings.ProcessExclusionsEnabled);
    WriteSetting(iniFile, L"ProcessExclusions", L"ProcessList"           , g_Settings.ProcessExclusions       );
}

/*!
 * \brief Load settings the the settings file path
 */
void ATLoadSettings() {
    AT_LOG_TRACE;
    std::wstring iniFile = ATSettingsFilePath();

    ReadSetting(iniFile, L"ListView"         , L"FontName"              , DEFAULT_FONT_NAME                 , g_Settings.FontName                );
    ReadSetting(iniFile, L"ListView"         , L"FontSize"              , DEFAULT_FONT_SIZE                 , g_Settings.FontSize                );
    ReadSetting(iniFile, L"ListView"         , L"FontStyle"             , DEFAULT_FONT_STYLE                , g_Settings.FontStyle               );
    ReadSetting(iniFile, L"ListView"         , L"FontColor"             , DEFAULT_FONT_COLOR                , g_Settings.FontColor               );
    ReadSetting(iniFile, L"ListView"         , L"BackgroundColor"       , DEFAULT_BG_COLOR                  , g_Settings.BackgroundColor         );
    ReadSetting(iniFile, L"Backtick"         , L"SimilarProcessGroups"  , DEFAULT_SIMILARPROCESSGROUPS      , g_Settings.SimilarProcessGroups    );
    ReadSetting(iniFile, L"General"          , L"PromptTerminateAll"    , DEFAULT_PROMPTTERMINATEALL        , g_Settings.PromptTerminateAll      );
    ReadSetting(iniFile, L"General"          , L"FuzzyMatchPercent"     , DEFAULT_FUZZYMATCHPERCENT         , g_Settings.FuzzyMatchPercent       );
    ReadSetting(iniFile, L"General"          , L"WindowTransparency"    , DEFAULT_TRANSPARENCY              , g_Settings.Transparency            );
    ReadSetting(iniFile, L"General"          , L"WindowWidthPercentage" , DEFAULT_WIDTH                     , g_Settings.WidthPercentage         );
    ReadSetting(iniFile, L"General"          , L"WindowHeightPercentage", DEFAULT_HEIGHT                    , g_Settings.HeightPercentage        );
    ReadSetting(iniFile, L"General"          , L"ShowColHeader"         , DEFAULT_SHOW_COL_HEADER           , g_Settings.ShowColHeader           );
    ReadSetting(iniFile, L"General"          , L"ShowColProcessName"    , DEFAULT_SHOW_COL_PROCESSNAME      , g_Settings.ShowColProcessName      );
    ReadSetting(iniFile, L"General"          , L"CheckForUpdates"       , DEFAULT_CHECKFORUPDATES           , g_Settings.CheckForUpdatesOpt      );
    ReadSetting(iniFile, L"ProcessExclusions", L"Enabled"               , DEFAULT_PROCESS_EXCLUSIONS_ENABLED, g_Settings.ProcessExclusionsEnabled);
    ReadSetting(iniFile, L"ProcessExclusions", L"ProcessList"           , DEFAULT_PROCESS_EXCLUSIONS        , g_Settings.ProcessExclusions       );

    // Clear the previous ProcessGroupsList
    g_Settings.ProcessGroupsList.clear();

    auto vs = Split(g_Settings.SimilarProcessGroups, L"|");
    for (auto& item : vs) {
        auto processes = Split(item, L"/");
        for (auto& processName : processes)
            processName = ToLower(processName);
        g_Settings.ProcessGroupsList.emplace_back(processes.begin(), processes.end());
    }

    // Process ProcessExclusions
    // Always split and convert to lower case, then it is easy while checking
    g_Settings.ProcessExclusionList.clear();
    g_Settings.ProcessExclusionList = Split(ToLower(g_Settings.ProcessExclusions), L"/");

    // Initialize additional settings
    g_AltBacktickWndInfo.hWnd   = nullptr;
    g_AltBacktickWndInfo.hOwner = nullptr;
}

/*!
 * \brief Save current senttings to the settings ini file path.
 */
void ATSaveSettings() {
    AT_LOG_TRACE;
    ATSettingsToFile(ATSettingsFilePath(true));
}

/*!
 * \brief Get the text of the given dialog item. Actually this is the wrapper on GetDlgItemText
 * 
 * \param hDlg          Dialog handle
 * \param nIDDlgItem    Dialog item
 * 
 * \return Dialog item text in std::wstring.
 */
std::wstring GetDlgItemTextEx(HWND hDlg, int nIDDlgItem) {
    int      textLength             = GetWindowTextLength(GetDlgItem(hDlg, nIDDlgItem));
    wchar_t* buffer                 = new wchar_t[textLength + 1];
    GetDlgItemTextW(hDlg, nIDDlgItem, buffer, textLength + 1);
    std::wstring result = buffer;
    delete[] buffer;
    return result;
}

/*!
 * \brief Save the given settings to the AltTab settings ini file and load the
 * modified settings to application (g_Settings)
 * 
 * \param[in] hDlg        AltTab settings dialog handle
 */
void ATApplySettings(HWND hDlg) {
    AT_LOG_TRACE;

    // Read settings from UI
    AltTabSettings settings;
    ATReadSettingsFromUI(hDlg, settings);
    ATLogSettings(settings);

    // Check if the settings are valid
    bool isValid = false;
    auto errorInfo = settings.IsValid(isValid);
    if (!isValid) {
        MessageBox(hDlg, errorInfo.second.c_str(), errorInfo.first.c_str(), MB_ICONERROR | MB_OK);
        return;
    }

    g_Settings = settings;

    // Save settings
    ATSaveSettings();

    // Load settings to reconstruct the ProcessGroupsList
    ATLoadSettings();

    // Disable Apply button after saving settings
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_APPLY), false);
}

/**
 * \brief Read settings from UI
 * 
 * \param[in]  hDlg        AltTab settings dialog handle
 * \param[out] settings    AltTabSettings
 */
void ATReadSettingsFromUI(HWND hDlg, AltTabSettings& settings) {
    settings.SimilarProcessGroups      = GetDlgItemTextEx  (hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS);
    settings.FuzzyMatchPercent         = GetDlgItemInt     (hDlg, IDC_EDIT_FUZZY_MATCH_PERCENT     , nullptr, FALSE);
    settings.Transparency              = GetDlgItemInt     (hDlg, IDC_EDIT_WINDOW_TRANSPARENCY     , nullptr, FALSE);
    settings.WidthPercentage           = GetDlgItemInt     (hDlg, IDC_EDIT_WINDOW_WIDTH_PERCENTAGE , nullptr, FALSE);
    settings.HeightPercentage          = GetDlgItemInt     (hDlg, IDC_EDIT_WINDOW_HEIGHT_PERCENTAGE, nullptr, FALSE);
    settings.PromptTerminateAll        = IsDlgButtonChecked(hDlg, IDC_CHECK_PROMPT_TERMINATE_ALL ) == BST_CHECKED;
    settings.ShowColHeader             = IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW_COL_HEADER      ) == BST_CHECKED;
    settings.ShowColProcessName        = IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW_COL_PROCESSNAME ) == BST_CHECKED;
    settings.ProcessExclusionsEnabled  = IsDlgButtonChecked(hDlg, IDC_CHECK_PROCESS_EXCLUSIONS   ) == BST_CHECKED;
    settings.ProcessExclusions         = GetDlgItemTextEx  (hDlg, IDC_EDIT_PROCESS_EXCLUSIONS    );
    int selectedIndex                  = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_CHECK_FOR_UPDATES));
    settings.CheckForUpdatesOpt        = AltTabSettings::CheckForUpdatesOptions[max(selectedIndex, 0)];
}

/**
 * \brief Log AltTab settings
 * 
 * \param[in] settings    AltTabSettings
 */
void ATLogSettings(const AltTabSettings& settings) {
    AT_LOG_TRACE;
    AT_LOG_DEBUG("=== AltTab Settings Begin ===");
    AT_LOG_DEBUG("SimilarProcessGroups    : [%s]", WStrToUTF8(settings.SimilarProcessGroups).c_str());
    AT_LOG_DEBUG("FuzzyMatchPercent       : [%d]", settings.FuzzyMatchPercent);
    AT_LOG_DEBUG("Transparency            : [%d]", settings.Transparency);
    AT_LOG_DEBUG("WidthPercentage         : [%d]", settings.WidthPercentage);
    AT_LOG_DEBUG("HeightPercentage        : [%d]", settings.HeightPercentage);
    AT_LOG_DEBUG("PromptTerminateAll      : [%d]", settings.PromptTerminateAll);
    AT_LOG_DEBUG("ShowColHeader           : [%d]", settings.ShowColHeader);
    AT_LOG_DEBUG("ShowColProcessName      : [%d]", settings.ShowColProcessName);
    AT_LOG_DEBUG("ProcessExclusionsEnabled: [%d]", settings.ProcessExclusionsEnabled);
    AT_LOG_DEBUG("ProcessExclusions       : [%s]", WStrToUTF8(settings.ProcessExclusions).c_str());
    AT_LOG_DEBUG("=== AltTab Settings End ===");
}

/*!
 * \brief Check if the settings are modified.
 * 
 * \param[in]  hDlg  AltTab settings dialog handle
 * 
 * \return true if the settings are modified, false otherwise
 */
bool AreSettingsModified(HWND hDlg) {
    AltTabSettings settings;
    ATReadSettingsFromUI(hDlg, settings);

    //ATLogSettings(settings);

    bool modified =
        settings.FuzzyMatchPercent        != g_Settings.FuzzyMatchPercent        ||
        settings.Transparency             != g_Settings.Transparency             ||
        settings.WidthPercentage          != g_Settings.WidthPercentage          ||
        settings.HeightPercentage         != g_Settings.HeightPercentage         ||
        settings.SimilarProcessGroups     != g_Settings.SimilarProcessGroups     ||
        settings.ShowColHeader            != g_Settings.ShowColHeader            ||
        settings.ShowColProcessName       != g_Settings.ShowColProcessName       ||
        settings.PromptTerminateAll       != g_Settings.PromptTerminateAll       ||
        settings.CheckForUpdatesOpt       != g_Settings.CheckForUpdatesOpt       ||
        settings.ProcessExclusionsEnabled != g_Settings.ProcessExclusionsEnabled ||
        settings.ProcessExclusions        != g_Settings.ProcessExclusions        ||
        false;

    return modified;
}

/**
 * \brief Available options of CheckForUpdates
 */
StringList AltTabSettings::CheckForUpdatesOptions = { L"Startup", L"Daily", L"Weekly", L"Never" };

/**
 * \brief Check if the given settings are valid.
 * 
 * \return true if settings are valid false otherwise.
 */
std::pair<std::wstring, std::wstring> AltTabSettings::IsValid(bool& valid) {
    const std::wregex pattern(L"^[^\\/:*?\"<>|]+.exe$");

    // Check similar process groups
    auto vs = Split(SimilarProcessGroups, L"|");
    for (int i = 0; i < vs.size(); ++i) {
        auto processes = Split(vs[i], L"/");
        for (auto& processName : processes) {
            if (!std::regex_match(processName, pattern)) {
                valid = false;
                return {
                    L"Invalid Similar Process Groups",
                    std::format(L"Similar Process Groups text contains invalid characters.\n"
                                 "A file name should not contain any of the following characters: \\ / : * ? \" < > | and ends with .exe.\n"
                                 "Found an invalid process name [{}] in group {}, please verify...", processName, i + 1)
                };
            }
        }
    }

    // Check exclude process list
    auto excludeProcessNames = Split(ProcessExclusions, L"/");
    for (auto& processName : excludeProcessNames) {
        if (!std::regex_match(processName, pattern)) {
            valid = false;
            return {
                L"Invalid Process Exclusions",
                std::format(L"Invalid process name [{}] in Process Exclusions, please verify..."
                "A file name should not contain any of the following characters: \\ / : * ? \" < > | and ends with .exe.", processName)
            };
        }
    }

    valid = true;
    return {};
}

/*!
 * \brief Initialize AltTab settings dialog controls with the given settings.
 * 
 * \param hDlg       AltTab settings dialog handle
 * \param settings   AltTabSettings
 */
void ATSettingsInitDialog(HWND hDlg, const AltTabSettings& settings) {
    SetDlgItemText    (hDlg, IDC_EDIT_SETTINGS_FILEPATH       , ATSettingsFilePath().c_str());
    SetDlgItemText    (hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS  , settings.SimilarProcessGroups.c_str());
 
    HFONT    hFont     = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Lucida Console");
    HWND     hEditBox1 = GetDlgItem(hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS);
    HWND     hEditBox2 = GetDlgItem(hDlg, IDC_EDIT_PROCESS_EXCLUSIONS);
 
    SendMessage(hEditBox1, WM_SETFONT     , (WPARAM)hFont    , TRUE);
    SendMessage(hEditBox2, WM_SETFONT     , (WPARAM)hFont    , TRUE);
 
    CheckDlgButton    (hDlg, IDC_CHECK_PROMPT_TERMINATE_ALL   , settings.PromptTerminateAll       ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton    (hDlg, IDC_CHECK_SHOW_COL_HEADER        , settings.ShowColHeader            ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton    (hDlg, IDC_CHECK_SHOW_COL_PROCESSNAME   , settings.ShowColProcessName       ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton    (hDlg, IDC_CHECK_PROCESS_EXCLUSIONS     , settings.ProcessExclusionsEnabled ? BST_CHECKED : BST_UNCHECKED);

    EnableWindow      (GetDlgItem(hDlg, IDC_EDIT_PROCESS_EXCLUSIONS), settings.ProcessExclusionsEnabled);
 
    SetDlgItemInt     (hDlg, IDC_EDIT_FUZZY_MATCH_PERCENT     , settings.FuzzyMatchPercent, FALSE);
    SendDlgItemMessage(hDlg, IDC_SPIN_FUZZY_MATCH_PERCENT     , UDM_SETRANGE                , 0, MAKELPARAM(100, 0));
    SendDlgItemMessage(hDlg, IDC_SPIN_FUZZY_MATCH_PERCENT     , UDM_SETPOS                  , 0, MAKELPARAM(settings.FuzzyMatchPercent, 0));
 
    SetDlgItemInt     (hDlg, IDC_EDIT_WINDOW_TRANSPARENCY     , settings.Transparency     , FALSE);
    SendDlgItemMessage(hDlg, IDC_SPIN_WINDOW_TRANSPARENCY     , UDM_SETRANGE                , 0, MAKELPARAM(255, 0));
    SendDlgItemMessage(hDlg, IDC_SPIN_WINDOW_TRANSPARENCY     , UDM_SETPOS                  , 0, MAKELPARAM(settings.Transparency, 0));
 
    SetDlgItemInt     (hDlg, IDC_EDIT_WINDOW_WIDTH_PERCENTAGE , settings.WidthPercentage  , FALSE);
    SendDlgItemMessage(hDlg, IDC_SPIN_WINDOW_WIDTH_PERCENTAGE , UDM_SETRANGE                , 0, MAKELPARAM(90, 10));
    SendDlgItemMessage(hDlg, IDC_SPIN_WINDOW_WIDTH_PERCENTAGE , UDM_SETPOS                  , 0, MAKELPARAM(settings.WidthPercentage, 0));
 
    SetDlgItemInt     (hDlg, IDC_EDIT_WINDOW_HEIGHT_PERCENTAGE, settings.HeightPercentage , FALSE);
    SendDlgItemMessage(hDlg, IDC_SPIN_WINDOW_HEIGHT_PERCENTAGE, UDM_SETRANGE                , 0, MAKELPARAM(90, 10));
    SendDlgItemMessage(hDlg, IDC_SPIN_WINDOW_HEIGHT_PERCENTAGE, UDM_SETPOS                  , 0, MAKELPARAM(settings.HeightPercentage, 0));
 
    SetDlgItemText    (hDlg, IDC_EDIT_PROCESS_EXCLUSIONS      , settings.ProcessExclusions.c_str());
 
    HWND hComboBox = GetDlgItem(hDlg, IDC_CHECK_FOR_UPDATES);
    for (auto& opt : AltTabSettings::CheckForUpdatesOptions) {
        ComboBox_AddString(hComboBox, opt.c_str());
    }
    ComboBox_SetCurSel(hComboBox, settings.GetCheckForUpdatesIndex());
 
    // Center the dialog on the screen
    int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    RECT dlgRect;
    GetWindowRect(hDlg, &dlgRect);
 
    int dlgWidth  = dlgRect.right  - dlgRect.left;
    int dlgHeight = dlgRect.bottom - dlgRect.top;
 
    int posX      = (screenWidth  - dlgWidth ) / 2;
    int posY      = (screenHeight - dlgHeight) / 2;
 
    SetWindowPos(hDlg, HWND_TOP, posX, posY, 0, 0, SWP_NOSIZE);
 
    // Set the dialog as an app window, otherwise not displayed in task bar
    SetWindowLong(hDlg, GWL_EXSTYLE, GetWindowLong(hDlg, GWL_EXSTYLE) | WS_EX_APPWINDOW);
 
    // Needs to be called after the dialog is shown
    bool settingsModified = AreSettingsModified(hDlg);
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_APPLY), settingsModified);
}
