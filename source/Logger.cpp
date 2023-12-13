#include "Logger.h"
#include "log4cpp/Appender.hh"
#include "Utils.h"
#include <filesystem>
#include "AltTabSettings.h"

std::shared_ptr<log4cpp::Category> gLogger;

void CreateLogger() {
    // Create an appender and a layout
#ifdef _AT_LOGGER
    EnableConsoleWindow();
    log4cpp::Appender* appender = new log4cpp::OstreamAppender("console", &std::cout);
#else
    std::filesystem::path logFilePath = ATSettingsDirPath();
    logFilePath.append("AltTab.log");

    log4cpp::Appender* appender = new log4cpp::FileAppender("FileAppender", logFilePath.string());
#endif // 0

    //appender->setLayout(new log4cpp::BasicLayout());
    // Create a pattern layout
    // %d{%Y-%m-%d %H:%M:%S,%l}
    log4cpp::PatternLayout* layout = new log4cpp::PatternLayout();
    layout->setConversionPattern("%d{%H:%M:%S,%l} [%-5p] %m%n");

    appender->setLayout(layout);

    // Create a category and add the appender to it
    gLogger.reset(&log4cpp::Category::getRoot(), [](auto&) { log4cpp::Category::shutdown(); });
    gLogger->setAppender(appender);

    // Set priority for root logger
    gLogger->setPriority(log4cpp::Priority::DEBUG);
}