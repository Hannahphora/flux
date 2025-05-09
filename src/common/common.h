#ifndef INCLUDE_COMMON_H
#define INCLUDE_COMMON_H

#include <stdlib.h>
#include <stdio.h>

// testing
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
// end testing

#define bool  _Bool
#define false 0
#define true  1

typedef unsigned char            u8;
typedef unsigned short          u16;
typedef unsigned int            u32;
typedef unsigned long long      u64;
typedef char                     i8;
typedef short                   i16;
typedef int                     i32;
typedef long long               i64;
typedef float                   f32;
typedef double                  f64;

#ifndef FLUX_FPRINTF
#define FLUX_FPRINTF fprintf
#endif // FLUX_FPRINTF

#ifndef FLUX_VFPRINTF
#define FLUX_VFPRINTF vfprintf
#endif // FLUX_VFPRINTF

#ifndef FLUX_ASSERT
#include <assert.h>
#define FLUX_ASSERT assert
#endif // FLUX_ASSERT

#ifndef FLUX_MALLOC
#define FLUX_MALLOC malloc
#endif // FLUX_MALLOC

#ifndef FLUX_REALLOC
#define FLUX_REALLOC realloc
#endif // FLUX_REALLOC

#ifndef FLUX_FREE
#define FLUX_FREE free
#endif // FLUX_FREE

// https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
#define FLUX_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) __attribute__ ((format (printf, STRING_INDEX, FIRST_TO_CHECK)))

#define FLUX_UNUSED(value) (void)(value)
#define FLUX_TODO(message) do { FLUX_FPRINTF(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, message); abort(); } while(0)
#define FLUX_UNREACHABLE(message) do { FLUX_FPRINTF(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message); abort(); } while(0)

#define FLUX_ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))
#define FLUX_ARRAY_GET(array, index) (FLUX_ASSERT((size_t)index < FLUX_ARRAY_LEN(array)), array[(size_t)index])

// basically pops an element from the beginning of a sized array
// useful for parsing cmd line args
#define flux_shift(xs, xs_sz) (FLUX_ASSERT((xs_sz) > 0), (xs_sz)--, *(xs)++)

#define flux_return_defer(value) do { result = (value); goto defer; } while(0)



#ifdef FLUX_HM_IMPL

#endif // FLUX_HM_IMPL

#ifdef FLUX_ALLOC_IMPL

#endif // FLUX_ALLOC_IMPL

#ifdef FLUX_LOG_IMPL

    typedef enum {
        FLUX_INFO,
        FLUX_WARNING,
        FLUX_ERROR,
        FLUX_NO_LOGS,
    } Flux_Log_Level;

    #ifndef FLUX_MIN_LOG_LEVEL
    #define FLUX_MIN_LOG_LEVEL FLUX_INFO
    #endif // FLUX_MIN_LOG_LEVEL

    void flux_log(Flux_Log_Level level, const char *fmt, ...) FLUX_PRINTF_FORMAT(2, 3)
    {
        if (level < FLUX_MIN_LOG_LEVEL) return;

        switch (level) {
        case FLUX_INFO:
            FLUX_FPRINTF(stderr, "[INFO] ");
            break;
        case FLUX_WARNING:
            FLUX_FPRINTF(stderr, "[WARNING] ");
            break;
        case FLUX_ERROR:
            FLUX_FPRINTF(stderr, "[ERROR] ");
            break;
        case FLUX_NO_LOGS: return;
        default:
            FLUX_UNREACHABLE("flux_log");
        }

        va_list args;
        va_start(args, fmt);
        FLUX_VFPRINTF(stderr, fmt, args);
        va_end(args);
        FLUX_FPRINTF(stderr, "\n");
    }

#endif // FLUX_LOG_IMPL

#endif // INCLUDE_COMMON_H

#ifndef COMMON_H_USE_FLUX_PREFIX_GUARD
#define COMMON_H_USE_FLUX_PREFIX_GUARD
    #ifndef COMMON_H_USE_FLUX_PREFIX

        #define FPRINTF                         FLUX_FPRINTF
        #define VFPRINTF                        FLUX_VFPRINTF
        #define PRINTF_FORMAT                   FLUX_PRINTF_FORMAT

        #define UNUSED                          FLUX_UNUSED
        #define TODO                            FLUX_TODO
        #define UNREACHABLE                     FLUX_UNREACHABLE
        #define ARRAY_LEN                       FLUX_ARRAY_LEN
        #define ARRAY_GET                       FLUX_ARRAY_GET
        #define shift                           flux_shift
        #define return_defer                    flux_return_defer

        // logging
        #define LOG_IMPL                        FLUX_LOG_IMPL
        #define Log_Level                       Flux_Log_Level
        #define INFO                            FLUX_INFO
        #define WARNING                         FLUX_WARNING
        #define ERROR                           FLUX_ERROR
        #define NO_LOGS                         FLUX_NO_LOGS
        #define MIN_LOG_LEVEL                   FLUX_MIN_LOG_LEVEL
        // NOTE: log is already taken in math.h, so prefix is kept for flux_log

    #endif // COMMON_H_USE_FLUX_PREFIX
#endif // COMMON_H_USE_FLUX_PREFIX_GUARD