#ifndef KORAX_LOG_H
#include <time.h>
#define KORAX_LOG_H
#define MIN_LOG_LEVEL 0
#define MAX_LOG_LEVEL 3
/* Higher numeric value = more verbose; log_set_level() gates by <=. */
typedef enum {
    LOG_ERROR = 0,
    LOG_WARN  = 1,
    LOG_INFO  = 2,
    LOG_DEBUG = 3
} log_level_t;

void log_set_level(log_level_t level);

void log_msg(log_level_t level, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

/* Rate-limited: accumulates a count and emits one summary WARN per
 * DECODE_REPORT_INTERVAL-second window rather than logging every call. */
int log_decode_failure(time_t now);
int log_enabled(log_level_t level);
int log_table_full(time_t now);
#endif
