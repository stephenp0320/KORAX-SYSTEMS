/*
 * Leveled logging to stderr, plus a rate limiter for decode-failure
 * warnings so a bad/noisy source can't flood the log at line rate.
 */
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
    if (format == NULL) return;
    if (level > g_log_level) return;

    va_list args;
    va_start(args, format);
    fprintf(stderr, "%s ", level_tag(level));
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

/* Coalesces decode failures instead of logging one line per bad frame:
 * counts accumulate silently until DECODE_REPORT_INTERVAL seconds have
 * elapsed since the first failure in the window, then a single summary
 * WARN is emitted and the window resets. Returns 1 when a summary was
 * actually logged (window closed), 0 while still accumulating. */
int log_decode_failure(time_t now){
    if (g_decode_fail_count == 0){
        g_decode_fail_since = now;
    }
     g_decode_fail_count++;

    if (now - g_decode_fail_since >= DECODE_REPORT_INTERVAL){
        log_msg(LOG_WARN, "%u decode failures in the last %lds", g_decode_fail_count, (long)(now - g_decode_fail_since));
        g_decode_fail_count = 0;
        return 1;
    }
    return 0;
}

/* Lets callers skip building an expensive log message (e.g. formatting
 * a findings dump) when the level wouldn't be printed anyway. */
int log_enabled(log_level_t level){
    return level <= g_log_level;
}
