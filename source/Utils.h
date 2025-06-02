#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>

bool EnableConsoleWindow();

std::string Trim(const std::string& str, const std::string& seps = " \t\r\n");

std::vector<std::string>  Split(const std::string& s , const std::string& seps  =  " \t");
std::vector<std::wstring> Split(const std::wstring& s, const std::wstring& seps = L" \t");

bool EqualsIgnoreCase(const std::string& s, const std::string& t);

bool EqualsIgnoreCase(const std::wstring& s, const std::wstring& t);

std::wstring ToLower(const std::wstring& s);

std::wstring ToUpper(const std::wstring& s);

std::string WStrToUTF8(const std::wstring& wstr);

std::wstring UTF8ToWStr(const std::string& utf8str);

std::string GetWindowTitleExA(HWND hWnd);

std::wstring GetWindowTitleExW(HWND hWnd);

bool InStr(const std::wstring& str, const std::wstring& substr);

double GetRatioA(const char* s1, const char* s2);

double GetPartialRatioA(const char* s1, const char* s2);

double GetRatioW(const wchar_t* s1, const wchar_t* s2);

double GetPartialRatioW(const wchar_t* s1, const wchar_t* s2);

double GetPartialRatioW(const std::wstring& s1, const std::wstring& s2);

int ScaleValueForDPI(int value, int dpi = 96);

int GetDPIForWindow(HWND hWnd);

bool is_auto_start_task_active_for_this_user();
bool create_auto_start_task_for_this_user(bool runElevated);
bool delete_auto_start_task_for_this_user();

std::wstring GetApplicationPath();

bool IsProcessElevated(const bool use_cached_value = true);

bool RestartApplication();

bool RunAsAdmin(const std::wstring& command, const std::wstring& args = L"");

void RelaunchAsAdminAndExit(const bool withElvatedArg);

bool IsTaskRunWithHighestPrivileges();

bool CheckSingleInstance(const std::wstring& mutexName);

bool InitializeCOM();

void UninitializeCOM();
