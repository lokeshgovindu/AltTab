#pragma once

#include <string>
#include <vector>
#include <wtypes.h>
#include <unordered_set>



struct AltTabWindowData {
    HWND          hWnd;
    HICON         hIcon;
    std::wstring  Title;
    std::wstring  ProcessName;
    DWORD         PID;
};

extern HINSTANCE                       g_hInstance;

extern std::vector<AltTabWindowData>   g_AltTabWindows;

extern bool                            g_IsAltTab;
extern bool                            g_IsAltBacktick;

HWND  ShowAltTabWindow(HWND& hAltTabWnd, int direction);

void  ATWListViewSelectItem(int rowNumber);

void  ATWListViewDeleteItem(int rowNumber);

int   ATWListViewGetSelectedItem();

void  ATWListViewPageDown();
