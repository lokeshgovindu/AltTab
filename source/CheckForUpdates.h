#include "framework.h"
#include <chrono>

void CheckForUpdates(bool quiteMode = false);
std::chrono::system_clock::time_point ReadLastCheckForUpdatesTS();
void WriteCheckForUpdatesTS(const std::chrono::system_clock::time_point& timestamp);
