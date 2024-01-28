#ifdef _AT_LOGGER
#include "Logger.h"
#include "log4cpp/Appender.hh"
#include "log4cpp/RollingFileAppender.hh"
#include <filesystem>
#include "AltTabSettings.h"
#endif // _AT_LOGGER

#ifdef _DEBUG
#include "Utils.h"
#endif // _DEBUG


#ifdef _AT_LOGGER

#pragma comment(lib, "ws2_32.lib")

std::shared_ptr<log4cpp::Category> g_Logger;

void CreateLogger() {
    // Create an appender and a layout
#if defined(_DEBUG)
    EnableConsoleWindow();
    log4cpp::Appender* appender = new log4cpp::OstreamAppender("console", &std::cout);
#else
    std::filesystem::path logFilePath = ATSettingsDirPath();
    logFilePath.append("AltTab.log");

    log4cpp::Appender* appender = new log4cpp::RollingFileAppender("FileAppender", logFilePath.string(), 5 * 1024 * 1024, 5);
#endif

    //appender->setLayout(new log4cpp::BasicLayout());
    // Create a pattern layout
    // %d{%Y-%m-%d %H:%M:%S,%l}
    // %t - threadID
    log4cpp::PatternLayout* layout = new log4cpp::PatternLayout();
    //layout->setConversionPattern("%d{%H:%M:%S,%l} [%-5p] [ThreadID-%5t] %m%n");   // Show thread-id
    layout->setConversionPattern("%d{%Y-%m-%d %H:%M:%S,%l} [%-5p] %m%n");           // Show date
    //layout->setConversionPattern("%d{%H:%M:%S,%l} [%-5p] %m%n");                  // Simple

    appender->setLayout(layout);

    // Create a category and add the appender to it
    g_Logger.reset(&log4cpp::Category::getRoot(), [](auto&) { log4cpp::Category::shutdown(); });
    g_Logger->setAppender(appender);

    // Set priority for root logger
    g_Logger->setPriority(log4cpp::Priority::DEBUG);
}

#endif // _AT_LOGGER
