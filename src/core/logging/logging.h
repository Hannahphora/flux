#ifndef INCLUDE_LOGGING_H
#define INCLUDE_LOGGING_H

typedef enum {
    FLUX_LOG_LEVEL_VERBOSE,
    FLUX_LOG_LEVEL_INFO,
    FLUX_LOG_LEVEL_WARNING,
    FLUX_LOG_LEVEL_ERROR,
} Flux_Log_Level;

#ifndef FLUX_MIN_LOG_LEVEL
#define FLUX_MIN_LOG_LEVEL FLUX_LOG_LEVEL_INFO
#endif // FLUX_MIN_LOG_LEVEL

void _flux_log(i8 *file, i32 line, Flux_Log_Level lvl, i8 *fmt, ...) __attribute__ ((format (printf, 4, 5)));

#endif // INCLUDE_LOGGING_H