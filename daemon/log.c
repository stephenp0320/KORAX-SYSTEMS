#include <stdio.h>
#include "daemon/log.h"
#include <stdarg.h>
#define DECODE_REPORT_INTERVAL 10
static log_level_t g_log_level = LOG_INFO;
static unsigned g_decode_fail_count = 0;
static time_t g_decode_fail_since = 0;

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

void log_decode_failure(time_t now){
    if (!now) return;

    if (g_decode_fail_count == 0){
        g_decode_fail_since = now;
        g_decode_fail_count++;
    }

    if (now - g_decode_fail_since >= DECODE_REPORT_INTERVAL){
        log_msg(LOG_WARN, "%u decode failures in the last %lds",
                g_decode_fail_count, (long)(now - g_decode_fail_since));
        g_decode_fail_count = 0;
    }
}



