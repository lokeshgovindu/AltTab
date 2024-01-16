#pragma once

const wchar_t* TT_SETTINGS_FILEPATH            = L"AltTab Settings file path";

const wchar_t* TT_SIMILAR_PROCESS_GROUPS       = LR"(Similar Process Groups:

Processes in a process group are separated by forward slash (/) and process groups
are separated by Pipe/Vertical Bar (|).
A file/process name can't contain any of the following characters: \ / : * ? " < > |

Example: notepad.exe/notepad++.exe|iexplore.exe/chrome.exe/firefox.exe|explorer.exe
)";

const wchar_t* TT_FUZZY_MATCH_PERCENT          = LR"(Fuzzy name match percent between search string and process name/title.
    100% means exact match
     80% would cause "files" to match "filas"
    and so on ...
)";

const wchar_t* TT_WINDOW_TRANSPARENCY          = LR"(Indicates the degree of transparency.
    0 - makes the window invisible.
    255 - makes it opaque.
    128 - makes it semi-transparent.
)";

const wchar_t* TT_WINDOW_WIDTH_PERCENT         = L"Main window width wrt percentage of screen width";

const wchar_t* TT_WINDOW_HEIGHT_PERCENT        = LR"(Main window maximum height wrt percentage of screen height.

If there are more number of processes which exceeds the WindowHeightMax
to accommodate all the tasks then resize the window to WindowHeightMax
according scroll bar presence.
)";

const wchar_t* TT_CHECK_FOR_UPDATES            = LR"(How frequently check for updates)";

const wchar_t* TT_PROMPT_TERMINATE_ALL         = LR"(Prompts for confirmation before terminating a window/process)";

const wchar_t* TT_SHOW_COLUMN_HEADER           = LR"(Show/hide column header of AltTab window listview)";

const wchar_t* TT_SHOW_COLUMN_PROCESS_NAME     = LR"(Show/hide process name column in AltTab window listview)";

const wchar_t* TT_CHECK_PROCESS_EXCLUSIONS     = LR"(Enable/disable process exclusions)";

const wchar_t* TT_EDIT_PROCESS_EXCLUSIONS      = LR"(Process exclusion list:
Process names are separated by forward slash (/).
Example: notepad.exe/iexplore.exe
)";

const wchar_t* TT_RESET_SETTINGS               = LR"(Reset all settings to defaults)";

const wchar_t* TT_APPLY_SETTINGS               = LR"(Save the modified settings to INI file.)";

const wchar_t* TT_OK_SETTINGS                  = LR"(Save the modified settings to INI file and close the dialog.)";

const wchar_t* TT_CANCEL_SETTINGS              = LR"(Don't save the modified settings to INI file and close dialog.)";
