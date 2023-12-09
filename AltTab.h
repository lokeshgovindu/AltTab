#pragma once

#include "resource.h"

LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wp, LPARAM lp);

LRESULT CALLBACK AltTabTrayIconProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK ATAboutDlgProc(HWND, UINT, WPARAM, LPARAM);

void ShowContextMenu(HWND hWnd, POINT pt);

void DestoryAltTabWindow();