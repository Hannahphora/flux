#ifndef INCLUDE_TYPES_H
#define INCLUDE_TYPES_H

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

#ifdef DA_IMPL

    #ifndef DA_INIT_CAP
    #define DA_INIT_CAP 64
    #endif // DA_INIT_CAP

    #ifndef DA_GROWTH_FACTOR
    #define  DA_GROWTH_FACTOR 1.5
    #endif // DA_GROWTH_FACTOR

    #define da_define(name, T)\
        typedef struct {\
            T *items;\
            size_t count;\
            size_t capacity;\
        } name
    // da_define

    #define da_reserve(da, expected_capacity)\
        do {\
            if ((expected_capacity) > (da)->capacity) {\
                if ((da)->capacity == 0) {\
                    (da)->capacity = DA_INIT_CAP;\
                }\
                while ((expected_capacity) > (da)->capacity) {\
                    (da)->capacity *= DA_GROWTH_FACTOR;\
                }\
                (da)->items = REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));\
                ASSERT((da)->items != NULL && "Memory allocation failed");\
            }\
        } while (0)
    // da_reserve

    #define da_append(da, item)\
        do {\
            da_reserve((da), (da)->count + 1);\
            (da)->items[(da)->count++] = (item);\
        } while (0)
    // da_append

    #define da_append_many(da, new_items, new_items_count)\
        do {\
            da_reserve((da), (da)->count + (new_items_count));\
            memcpy((da)->items + (da)->count, (new_items), (new_items_count)*sizeof(*(da)->items));\
            (da)->count += (new_items_count);\
        } while (0)
    // da_append_many

    #define da_resize(da, new_size)\
        do {\
            da_reserve((da), new_size);\
            (da)->count = (new_size);\
        } while (0)
    // da_resize

    #define da_last(da) (da)->items[(ASSERT((da)->count > 0), (da)->count-1)]

    #define da_remove_unordered(da, i)\
        do {\
            size_t j = (i);\
            ASSERT(j < (da)->count);\
            (da)->items[j] = (da)->items[--(da)->count];\
        } while(0)
    // da_remove_unordered

    #define da_free(da)\
        do {\
            FREE((da).items);\
            (da).items = NULL;\
            (da).count = 0;\
            (da).capacity = 0;\
        } while (0)
    // da_free

#endif // DA_IMPL

#ifdef HM_IMPL
    
#endif // HM_IMPL

#endif // INCLUDE_TYPES_H