#ifndef INCLUDE_COMMON_H
#define INCLUDE_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#define bool  _Bool
#define true  1
#define false 0

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

#ifndef FLUX_MALLOC
    #define FLUX_MALLOC malloc
#endif // FLUX_MALLOC

#ifndef FLUX_REALLOC
    #define FLUX_REALLOC realloc
#endif // FLUX_REALLOC

#ifndef FLUX_FREE
    #define FLUX_FREE free
#endif // FLUX_FREE

#ifndef FLUX_MEMCPY
    #define FLUX_MEMCPY memcpy
#endif // FLUX_MEMCPY

#define FLUX_UNUSED(value) (void)(value)
#define FLUX_TODO(msg) do { FLUX_LOG(FLUX_LOG_LEVEL_ERROR_BIT, msg); abort(); } while(0)
#define FLUX_ASSERT(x) (!(x) && FLUX_LOG(FLUX_LOG_LEVEL_ERROR_BIT, "Assertion failed: %s", #x) && (abort(), 1))

// NOTE: this is a compiler hint for optimisation, it is not for assertion
#define FLUX_UNREACHABLE do { __builtin_unreachable(); } while(0)

#define FLUX_ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))
#define FLUX_ARRAY_GET(array, index) (FLUX_ASSERT((size_t)index < FLUX_ARRAY_LEN(array)), array[(size_t)index])

// basically pops an element from the beginning of a sized array, useful for parsing cmd line args
#define flux_shift(xs, xs_sz) (FLUX_ASSERT((xs_sz) > 0), (xs_sz)--, *(xs)++)

#define flux_return_defer(value) do { retval = (value); goto defer; } while(0)

#endif // INCLUDE_COMMON_H

#ifndef COMMON_H_FLUX_PREFIX_GUARD
#define COMMON_H_FLUX_PREFIX_GUARD
    #ifndef COMMON_H_FLUX_PREFIX

        #define FPRINTF                         FLUX_FPRINTF
        #define MALLOC                          FLUX_MALLOC
        #define REALLOC                         FLUX_REALLOC
        #define FREE                            FLUX_FREE

        #define UNUSED                          FLUX_UNUSED
        #define TODO                            FLUX_TODO
        #define ASSERT                          FLUX_ASSERT
        
        #define UNREACHABLE                     FLUX_UNREACHABLE

        #define ARRAY_LEN                       FLUX_ARRAY_LEN
        #define ARRAY_GET                       FLUX_ARRAY_GET

        #define shift                           flux_shift
        #define return_defer                    flux_return_defer

    #endif // COMMON_H_FLUX_PREFIX
#endif // COMMON_H_FLUX_PREFIX_GUARD