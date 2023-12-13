#pragma once

#include "resource.h"

extern HWND           g_hWndTrayIcon;              // AltTab tray icon

LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wp, LPARAM lp);

LRESULT CALLBACK AltTabTrayIconProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK ATAboutDlgProc(HWND, UINT, WPARAM, LPARAM);

void ShowContextMenu(HWND hWnd, POINT pt);

void TrayContextMenuItemHandler(HWND hWnd, HMENU hSubMenu, UINT menuItemId);

void DestoryAltTabWindow();

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

