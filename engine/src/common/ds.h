/*
    NOTE: data structures depends on allocators.h for allocation
*/

#ifndef INCLUDE_DS_H
#define INCLUDE_DS_H

#include "common.h"

#ifdef FLUX_DYNAMIC_ARRAY

    #ifndef FLUX_DA_INIT_CAP
        #define FLUX_DA_INIT_CAP 128
    #endif // FLUX_DA_INIT_CAP

    #ifndef FLUX_DA_GROWTH_FACTOR
        #define  FLUX_DA_GROWTH_FACTOR 1.5
    #endif // FLUX_DA_GROWTH_FACTOR

    #define flux_da_define(name, T)\
        typedef struct {\
            T *items;\
            size_t count;\
            size_t capacity;\
        } name

    #define flux_da_reserve(da, expected_capacity)\
        do {\
            if ((expected_capacity) > (da)->capacity) {\
                if ((da)->capacity == 0) {\
                    (da)->capacity = FLUX_DA_INIT_CAP;\
                }\
                while ((expected_capacity) > (da)->capacity) {\
                    (da)->capacity *= FLUX_DA_GROWTH_FACTOR;\
                }\
                (da)->items = FLUX_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));\
                FLUX_ASSERT((da)->items != NULL && "Memory allocation failed");\
            }\
        } while (0)

    #define flux_da_append(da, item)\
        do {\
            flux_da_reserve((da), (da)->count + 1);\
            (da)->items[(da)->count++] = (item);\
        } while (0)

    #define flux_da_append_many(da, new_items, new_items_count)\
        do {\
            flux_da_reserve((da), (da)->count + (new_items_count));\
            FLUX_MEMCPY((da)->items + (da)->count, (new_items), (new_items_count)*sizeof(*(da)->items));\
            (da)->count += (new_items_count);\
        } while (0)

    #define flux_da_resize(da, new_size)\
        do {\
            flux_da_reserve((da), new_size);\
            (da)->count = (new_size);\
        } while (0)

    #define flux_da_last(da) (da)->items[(FLUX_ASSERT((da)->count > 0), (da)->count-1)]

    #define flux_da_remove_unordered(da, i)\
        do {\
            size_t j = (i);\
            FLUX_ASSERT(j < (da)->count);\
            (da)->items[j] = (da)->items[--(da)->count];\
        } while(0)

    #define flux_da_free(da)\
        do {\
            FLUX_FREE((da).items);\
            (da).items = NULL;\
            (da).count = 0;\
            (da).capacity = 0;\
        } while (0)
    
#endif // FLUX_DYNAMIC_ARRAY

#ifdef FLUX_HASH_MAP



#endif // FLUX_HASH_MAP

#ifdef FLUX_STRING_BUILDER
    #ifndef FLUX_DYNAMIC_ARRAY
        #error "FLUX_STRING_BUILDER requires FLUX_DYNAMIC_ARRAY to be defined"
    #endif // FLUX_DYNAMIC_ARRAY

    flux_da_define(Flux_String_Builder, char);

    int flux_sb_appendf(Flux_String_Builder *sb, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

     // append a sized buffer to a string builder
    #define flux_sb_append_buf(sb, buf, size) flux_da_append_many(sb, buf, size)

    // append a null-terminated string to a string builder
    #define flux_sb_append_cstr(sb, cstr)\
        do {\
            const char *s = (cstr);\
            size_t n = strlen(s);\
            flux_da_append_many(sb, s, n);\
        } while (0)

    // appends a single null character to string builder, for use as c string
    #define flux_sb_append_null(sb) flux_da_append_many(sb, "", 1)

    #define flux_sb_free(sb) flux_da_free(sb)

#endif // FLUX_STRING_BUILDER

#endif // INCLUDE_DS_H

#ifndef DS_H_FLUX_PREFIX_GUARD
#define DS_H_FLUX_PREFIX_GUARD
    #ifndef DS_H_FLUX_PREFIX

    #define DYNAMIC_ARRAY                   FLUX_DYNAMIC_ARRAY
    #define DA_INIT_CAP                     FLUX_DA_INIT_CAP
    #define DA_GROWTH_FACTOR                FLUX_DA_GROWTH_FACTOR
    #define da_define                       flux_da_define
    #define da_reserve                      flux_da_reserve
    #define da_append                       flux_da_append
    #define da_append_many                  flux_da_append_many
    #define da_resize                       flux_da_resize
    #define da_last                         flux_da_last
    #define da_remove_unordered             flux_da_remove_unordered
    #define da_free                         flux_da_free

    #define HASH_MAP                        FLUX_HASH_MAP


    #define STRING_BUILDER                  FLUX_STRING_BUILDER


    #endif // DS_H_FLUX_PREFIX
#endif // DS_H_FLUX_PREFIX_GUARD