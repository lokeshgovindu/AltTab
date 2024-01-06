#pragma once

#include <string>
#include <vector>
#include <wtypes.h>
#include <unordered_set>

struct AltTabWindowData {
    HWND          hWnd;
    HWND          hOwner;
    HICON         hIcon;
    std::wstring  Title;
    std::wstring  ProcessName;
    DWORD         PID;
};

HWND CreateAltTabWindow();

HWND ShowAltTabWindow(HWND& hAltTabWnd, int direction);

void RefreshAltTabWindow();

void ATWListViewSelectItem(int rowNumber);
void ATWListViewSelectNextItem();
void ATWListViewSelectPrevItem();

void ATWListViewDeleteItem(int rowNumber);

int  ATWListViewGetSelectedItem();

void ATWListViewPageDown();

void ShowContextMenuAtItemCenter();

void ShowContextMenu(HWND hWnd, POINT pt);

void SetAltTabActiveWindow();

