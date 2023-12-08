#pragma once

#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/PatternLayout.hh>
#include <memory>

extern std::shared_ptr<log4cpp::Category> gLogger;

#if defined(_AT_LOGGER)

#define AT_LOG(level, message)   gLogger->log(level, "[%-21s | %3d] %s", __FUNCTION__, __LINE__, message)

#define AT_LOG_DEBUG(message)    AT_LOG(log4cpp::Priority::DEBUG, message)
#define AT_LOG_INFO(message)     AT_LOG(log4cpp::Priority::INFO,  message)
#define AT_LOG_WARN(message)     AT_LOG(log4cpp::Priority::WARN,  message)
#define AT_LOG_ERROR(message)    AT_LOG(log4cpp::Priority::ERROR, message)
#define AT_LOG_ALERT(message)    AT_LOG(log4cpp::Priority::ALERT, message)
#define AT_LOG_TRACE             AT_LOG_DEBUG("")

#else

#define AT_LOG(level, message)   ;

#define AT_LOG_DEBUG(message)    AT_LOG(log4cpp::Priority::DEBUG, message)
#define AT_LOG_INFO(message)     AT_LOG(log4cpp::Priority::INFO,  message)
#define AT_LOG_WARN(message)     AT_LOG(log4cpp::Priority::WARN,  message)
#define AT_LOG_ERROR(message)    AT_LOG(log4cpp::Priority::ERROR, message)
#define AT_LOG_ALERT(message)    AT_LOG(log4cpp::Priority::ALERT, message)
#define AT_LOG_TRACE             AT_LOG_DEBUG("")

#endif // _AT_LOGGER

void CreateLogger();
