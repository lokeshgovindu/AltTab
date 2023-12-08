#include "Logger.h"
#include "log4cpp/Appender.hh"
#include "Utils.h"

std::shared_ptr<log4cpp::Category> gLogger;

void CreateLogger() {
    // Create an appender and a layout
#if 1
    EnableConsoleWindow();
    log4cpp::Appender* appender = new log4cpp::OstreamAppender("console", &std::cout);
#else
    log4cpp::Appender* appender = new log4cpp::FileAppender("FileAppender", "AltTab.log");
#endif // 0

    //appender->setLayout(new log4cpp::BasicLayout());
    // Create a pattern layout
    // %d{%Y-%m-%d %H:%M:%S,%l}
    log4cpp::PatternLayout* layout = new log4cpp::PatternLayout();
    layout->setConversionPattern("%d [%-5p] %c: %m%n");

    appender->setLayout(layout);

    // Create a category and add the appender to it
    gLogger.reset(&log4cpp::Category::getRoot(), [](auto&) { log4cpp::Category::shutdown(); });
    gLogger->setAppender(appender);

    // Set priority for root logger
    gLogger->setPriority(log4cpp::Priority::DEBUG);
}