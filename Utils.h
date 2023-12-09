#pragma once
#include <vector>
#include <string>

bool EnableConsoleWindow();

std::vector<std::wstring> Split(const std::wstring& s, const std::wstring& seps = L" \t");

bool EqualsIgnoreCase(const std::string& s, const std::string& t);
bool EqualsIgnoreCase(const std::wstring& s, const std::wstring& t);
std::wstring ToLower(const std::wstring& s);
std::wstring ToUpper(const std::wstring& s);