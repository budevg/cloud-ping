#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <stdarg.h>
#include <string.h>

#include "logging.h"

LogLevel log_level_g = LOG_INFO;

static const char* log_level_names[] = {"ERROR", "WARNING", "NOTICE", "INFO", "DEBUG"};

void log_set_level(LogLevel level)
{
  log_level_g = level;
}

void log_write(LogLevel level, const char* func_name, int line_count, const char *fmt, ...)
{
  struct timeval tv;
  va_list args;
  char template_buf[4096];
  char func_name_buf[1024];
  const char *func_name_p;
  pid_t tid;
  const char *func_name_start = NULL;
  const char *func_name_end = NULL;

  if (level > log_level_g) {
    return;
  }

  func_name_start = strchr(func_name, ':');
  if (func_name_start != NULL) {
    while (func_name_start > func_name && *func_name_start != ' ') {
      func_name_start--;
    }
    func_name_end = strchr(func_name, '(');
  }


  tid = syscall(SYS_gettid);

  gettimeofday(&tv, NULL);
  if (func_name_start != NULL && func_name_end != NULL) {
    snprintf(func_name_buf, func_name_end - func_name_start, "%s", func_name_start+1);
    func_name_p = func_name_buf;
  }
  else {
    func_name_p = func_name;
  }


  snprintf(template_buf, sizeof(template_buf), "%-8s %ld.%.6ld [%d]: %-5d:%-36s %s",
           log_level_names[level],
           tv.tv_sec, tv.tv_usec, tid, line_count, func_name_p , fmt);
  va_start(args, fmt);
  vprintf(template_buf, args);
  va_end(args);
}
