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

HWND  CreateAltTabWindow();

HWND  ShowAltTabWindow(HWND& hAltTabWnd, int direction);

void  HideAltTabWindow(HWND& hAltTabWnd);

void  DestoryAltTabWindow();

BOOL  AddNotificationIcon(HWND hWnd);

void  ATWListViewSelectItem(int rowNumber);

void  ATWListViewDeleteItem(int rowNumber);

int   ATWListViewGetSelectedItem();

void  ATWListViewPageDown();
