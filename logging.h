#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <stdio.h>

enum LogLevel {
  LOG_ERROR = 0,
  LOG_WARNING,
  LOG_NOTICE,
  LOG_INFO,
  LOG_DEBUG,
};

extern LogLevel log_level_g;

void log_write(LogLevel level, const char* func_name, int line_count, const char *fmt, ...);
void log_set_level(LogLevel level);

#define log_println(fmt, ...)    printf(fmt "\n", ##__VA_ARGS__)
#define log_notice(fmt, ...) log_write(LOG_NOTICE, __PRETTY_FUNCTION__, __LINE__, fmt "\n", ##__VA_ARGS__)
#define log_info(fmt, ...)   log_write(LOG_INFO, __PRETTY_FUNCTION__, __LINE__, fmt "\n", ##__VA_ARGS__)
#define log_debug(fmt, ...)  log_write(LOG_DEBUG, __PRETTY_FUNCTION__, __LINE__, fmt "\n", ##__VA_ARGS__)
#define log_warn(fmt, ...)  log_write(LOG_WARNING, __PRETTY_FUNCTION__, __LINE__, fmt "\n", ##__VA_ARGS__)
#define log_error(fmt, ...)  log_write(LOG_ERROR, __PRETTY_FUNCTION__, __LINE__, fmt "\n", ##__VA_ARGS__)

#endif /* _LOGGING_H_ */
