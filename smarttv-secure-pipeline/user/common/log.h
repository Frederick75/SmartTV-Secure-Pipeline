#pragma once
#include <cstdio>
#include <cstdarg>

inline void log_print(const char* lvl, const char* fmt, ...) {
  std::va_list ap;
  va_start(ap, fmt);
  std::fprintf(stderr, "[%s] ", lvl);
  std::vfprintf(stderr, fmt, ap);
  std::fprintf(stderr, "\n");
  va_end(ap);
}

#define LOGI(...) log_print("I", __VA_ARGS__)
#define LOGW(...) log_print("W", __VA_ARGS__)
#define LOGE(...) log_print("E", __VA_ARGS__)
