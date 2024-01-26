#pragma once

#include "resource.h"
#include <string>

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
bool RunAtStartup(bool flag);

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

void ShowCustomToolTip(LPCWSTR tooltipText);
