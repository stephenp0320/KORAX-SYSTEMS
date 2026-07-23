#include <stdio.h>
#include "daemon/log.h"
#include <stdarg.h>

static log_level_t g_log_level = LOG_INFO;

static const char *level_tag(log_level_t level){
    switch (level){
        case LOG_ERROR:
            return "ERROR";
        case LOG_WARN:
            return "WARN";
        case LOG_INFO:
            return "INFO";
        case LOG_DEBUG:
            return "DEBUG";
        default:
            return "?";
    }
}

void log_set_level(log_level_t level){
    g_log_level = level;
}

void log_msg(log_level_t level, const char *format, ...){
    if (!level || !format){
        return;
    }

    if (level > g_log_level){
        return;
    }
    va_list args;
    va_start(args, format);
    fprintf(stderr, "%s ", level_tag(level));
    if (!vprintf(format, args)){
        return;
    }
    fprintf(stderr, "\n");
    va_end(args);
}



