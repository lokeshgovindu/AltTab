#pragma once

#include "resource.h"
#include <string>

struct ToolTipInfo {
    std::wstring  ToolTipText;
    int           Duration;
};

/*!
 * @brief General settings of the application.
 * When the application starts, it checks the current user privileges.
 */
struct GeneralSettings {
    bool IsProcessElevated; // Is the application running with elevated privileges
    bool IsTaskElevated;    // Is the application set to run elevated (Run with highest privileges in the task options)
    bool IsRunAtStartup;    // Is the application set to run at startup
};

LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wp, LPARAM lp);

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK ATAboutDlgProc(HWND, UINT, WPARAM, LPARAM);

void ShowTrayContextMenu(HWND hWnd, POINT pt);

void TrayContextMenuItemHandler(HWND hWnd, HMENU hSubMenu, UINT menuItemId);

void DestoryAltTabWindow(bool activate = false);

void ToggleCheckState(HMENU hMenu, UINT menuItemID);

UINT GetCheckState(HMENU hMenu, UINT menuItemID);

void SetCheckState(HMENU hMenu, UINT menuItemID, UINT fState);

/**
 * Modify the status of RunAtStartup
 * 
 * \param flag  true to make the application run at startup otherwise false.
 * \return Success if the function is successful otherwise false
 */
bool RunAtStartup(const bool runAtStartup, const bool withHighestPrivileges);

bool IsRunAtStartup();

BOOL CALLBACK EnumWindowsProcNAT(HWND hwnd, LPARAM lParam);

bool IsNativeATWDisplayed();

void ActivateWindow(HWND hWnd);

BOOL IsHungAppWindowEx(HWND hwnd);

void ShowHelpWindow();

void ShowReadMeWindow();

void ShowReleaseNotesWindow();

std::wstring GetAppDirPath();

void LogLastErrorInfo();

void CreateCustomToolTip();

void ShowCustomToolTip(const std::wstring& tooltipText, int duration = 3000);

void CALLBACK HideCustomToolTip(HWND hWnd = nullptr, UINT uMsg = 0, UINT_PTR idEvent = 0, DWORD dwTime = 0);
