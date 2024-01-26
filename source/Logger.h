#pragma once

#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/PatternLayout.hh>
#include <memory>
#include "GlobalData.h"

#if defined(_AT_LOGGER)

template <typename... Args>
std::string FormatString(const char* fmt, const Args&... args) {
    int size = snprintf(nullptr, 0, fmt, args...);

    // Allocate a buffer to hold the formatted string
    std::string message(size + 1, '\0');

    // Format the string into the buffer
    snprintf(&message[0], size + 1, fmt, args...);

    return message;
}

#define AT_LOG(level, fmt, ...)   g_Logger->log(level, "[%-27s | %4d] %s", __FUNCTION__, __LINE__, FormatString(fmt, ##__VA_ARGS__).c_str())

#define AT_LOG_DEBUG(fmt, ...)    AT_LOG(log4cpp::Priority::DEBUG, fmt, ##__VA_ARGS__)
#define AT_LOG_INFO(fmt, ...)     AT_LOG(log4cpp::Priority::INFO,  fmt, ##__VA_ARGS__)
#define AT_LOG_WARN(fmt, ...)     AT_LOG(log4cpp::Priority::WARN,  fmt, ##__VA_ARGS__)
#define AT_LOG_ERROR(fmt, ...)    AT_LOG(log4cpp::Priority::ERROR, fmt, ##__VA_ARGS__)
#define AT_LOG_ALERT(fmt, ...)    AT_LOG(log4cpp::Priority::ALERT, fmt, ##__VA_ARGS__)
#define AT_LOG_TRACE              AT_LOG_DEBUG("")

#else

#define AT_LOG(level, message)
#define AT_LOG_DEBUG(message, ...)
#define AT_LOG_INFO(message, ...)
#define AT_LOG_WARN(message, ...)
#define AT_LOG_ERROR(message, ...)
#define AT_LOG_ALERT(message, ...)
#define AT_LOG_TRACE

#endif // _AT_LOGGER

void CreateLogger();
