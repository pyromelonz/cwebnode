#ifndef LOG_H
#define LOG_H

#include "types.h"
#include "config.h"

void log_printf(const char* fmt, ...);

void log_msg(err_t status, const char* const fmt, ...);

err_t log_get_error();

char log_init();

void log_term();


#ifdef LOG_VERBOSITY //hoping this is omitted by compiler when level < LOG_VERBOSITY
#define LOG(level, ...) \
    do { if ((level) >= LOG_VERBOSITY) log_msg(level, __VA_ARGS__); } while (0)
#else
#define LOG(level, ...) log_msg(level, __VA_ARGS__)
#endif

#endif //LOG_H
