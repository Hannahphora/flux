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

#define FLUX_LOG(lvl, msg, ...) do { _flux_log(__FILE__, __LINE__, lvl, msg,  __VA_ARGS__); } while(0)

#endif // INCLUDE_LOGGING_H

#ifndef LOGGING_H_FLUX_PREFIX_GUARD
#define LOGGING_H_FLUX_PREFIX_GUARD
    #ifndef LOGGING_H_FLUX_PREFIX

        #define LOG_IMPL                        FLUX_LOG_IMPL
        #define Log_Level                       Flux_Log_Level
        #define INFO                            FLUX_INFO
        #define WARNING                         FLUX_WARNING
        #define ERROR                           FLUX_ERROR
        #define NO_LOGS                         FLUX_NO_LOGS
        #define MIN_LOG_LEVEL                   FLUX_MIN_LOG_LEVEL

    #endif // LOGGING_H_FLUX_PREFIX
#endif // LOGGING_H_FLUX_PREFIX_GUARD