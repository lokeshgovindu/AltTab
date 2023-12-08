#pragma once
#include <vector>
#include <string>

bool EnableConsoleWindow();

std::vector<std::wstring> Split(const std::wstring& s, const std::wstring& seps = L" \t");