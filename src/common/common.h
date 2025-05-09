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

#ifdef _WIN32
    // windows specific

    #define WIN32_LEAN_AND_MEAN
    #define _WINUSER_
    #define _WINGDI_
    #define _IMM_
    #define _WINCON_
    #include <windows.h>
    #include <direct.h>
    #include <shellapi.h>

    #define FLUX_LINE_END "\r\n"
    #define FLUX_INVALID_FILE INVALID_HANDLE_VALUE
    #define FLUX_INVALID_PROC INVALID_HANDLE_VALUE
    typedef HANDLE Flux_File;
    typedef HANDLE Flux_Proc;
    
    

#else
    // linux specific

    #include <sys/types.h>
    #include <sys/wait.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <dirent.h>

    #define FLUX_LINE_END "\n"
    #define FLUX_INVALID_FILE (-1)
    #define FLUX_INVALID_PROC (-1)
    typedef int Flux_File;
    typedef int Flux_Proc;

#endif

typedef u16 Flux_Log_Level;
typedef enum Flux_Log_Level_Bits {
    FLUX_LOG_LEVEL_VERBOSE_BIT  = 0x0001,
    FLUX_LOG_LEVEL_INFO_BIT     = 0x0010,
    FLUX_LOG_LEVEL_WARNING_BIT  = 0x0100,
    FLUX_LOG_LEVEL_ERROR_BIT    = 0x1000,
} Flux_Log_Level_Bits;

bool flux_fprintf(Flux_File dest, i8 *fmt, ...) __attribute__ ((format (printf, 3, 4)));
bool flux_sprintf(i8 *dest, u32 dest_len, i8 *fmt, ...) __attribute__ ((format (printf, 3, 4)));

#ifndef FLUX_MIN_LOG_LEVEL
#define FLUX_MIN_LOG_LEVEL FLUX_LOG_LEVEL_INFO_BIT
#endif // FLUX_MIN_LOG_LEVEL

static void
_flux_log(i8 *caller_file, i32 caller_line, Flux_Log_Level lvl, i8 *fmt, ...)
__attribute__ ((format (printf, 4, 5)))
{
    if (lvl < FLUX_MIN_LOG_LEVEL)
        return;
    
    // TEMPORARY! replace fprintf with a proper method
    
    if (lvl & FLUX_LOG_LEVEL_VERBOSE_BIT) fprintf(stderr, "[");
    if (lvl & FLUX_LOG_LEVEL_INFO_BIT)
    if (lvl & FLUX_LOG_LEVEL_VERBOSE_BIT)

    switch (level) {
    case lvl:
        FLUX_FPRINTF(stderr, "[INFO] ");
        break;
    case FLUX_WARNING:
        FLUX_FPRINTF(stderr, "[WARNING] ");
        break;
    case FLUX_ERROR:
        FLUX_FPRINTF(stderr, "[ERROR] ");
        break;
    case FLUX_NO_LOGS:
        return;
    default:
        FLUX_UNREACHABLE("flux_log");
    }

}

#define FLUX_LOG(lvl, msg, ...) do { _flux_log(__FILE__, __LINE__, lvl, msg,  __VA_ARGS__); } while(0)
#define FLUX_UNUSED(value) (void)(value)
#define FLUX_TODO(msg) do { FLUX_LOG(FLUX_LOG_LEVEL_ERROR_BIT, msg); abort(); } while(0)
#define FLUX_UNREACHABLE(msg) do { FLUX_LOG(FLUX_LOG_LEVEL_ERROR_BIT, msg); abort(); } while(0)
#define FLUX_ASSERT(x) (!(x) && FLUX_LOG(FLUX_LOG_LEVEL_ERROR_BIT, "Assertion failed: %s", #x) && (abort(), 1))

#define FLUX_ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))
#define FLUX_ARRAY_GET(array, index) (FLUX_ASSERT((size_t)index < FLUX_ARRAY_LEN(array)), array[(size_t)index])

// basically pops an element from the beginning of a sized array
// useful for parsing cmd line args
#define flux_shift(xs, xs_sz) (FLUX_ASSERT((xs_sz) > 0), (xs_sz)--, *(xs)++)

#define flux_return_defer(value) do { result = (value); goto defer; } while(0)



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

    void flux_log(Flux_Log_Level level, const char *fmt, ...)
    {
        if (level < FLUX_MIN_LOG_LEVEL)
            return;

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
        case FLUX_NO_LOGS:
            return;
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