#pragma once

#include <string>
#include <vector>
#include <wtypes.h>
#include <unordered_set>

#define CLASS_NAME   L"__AltTab_WndCls__"
#define WINDOW_NAME  L"AltTab Window"

struct AltTabWindowData {
    HWND          hWnd;
    HWND          hOwner;
    HICON         hIcon;
    std::wstring  Title;
    std::wstring  ProcessName;
    std::wstring  FullPath;
    DWORD         PID;
};

/*!
 * \brief Register AltTab window class
 * 
 * \return Return true if the class is registered successfully otherwise false.
 */
bool RegisterAltTabWindow();

/*!
 * \brief Create AltTab main window
 */
HWND CreateAltTabWindow();

/**
 * \brief Show AltTab window
 * 
 * \param hAltTabWnd AltTab window handle
 * \param direction  Direction which tells to select next or previous item
 * \return 
 */
HWND ShowAltTabWindow(HWND& hAltTabWnd, int direction);

/**
 * \brief Clear the existing items and re-create the AltTab window with new windows list.
 */
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

/**
 * \brief Translate the virtual key code to a character value.
 * 
 * \param[in]  uCode    The virtual key code or scan code value of the key.
 * \param[out] vkCode   Either a virtual-key code or a character value.
 * 
 * \return Return true if the given uCode is a printable character otherwise false.
 */
bool ATMapVirtualKey(UINT uCode, wchar_t& ch);
