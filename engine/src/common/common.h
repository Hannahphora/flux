#ifndef INCLUDE_COMMON_H
#define INCLUDE_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#define DA_IMPL
#include "ds.h"

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

    #define FLUX_NEWLINE "\r\n"
    #define FLUX_INVALID_FILE INVALID_HANDLE_VALUE
    typedef HANDLE Flux_File;
    #define FLUX_INVALID_PROC INVALID_HANDLE_VALUE
    typedef HANDLE Flux_Proc;

    struct dirent { char d_name[MAX_PATH+1]; };
    typedef struct DIR DIR;
    static DIR *opendir(const char *dirpath);
    static struct dirent *readdir(DIR *dirp);
    static int closedir(DIR *dirp);

    char *flux_win32_error_message(DWORD err);
#else
    // linux specific
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <dirent.h>

    #define FLUX_NEWLINE "\n"
    #define FLUX_INVALID_FILE (-1)
    typedef int Flux_File;
    #define FLUX_INVALID_PROC (-1)
    typedef int Flux_Proc;
#endif

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

// STRING BUILDER
    da_define(Flux_String_Builder, char);

    int flux_sb_appendf(Flux_String_Builder *sb, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

    // append a sized buffer to a string builder
    #define flux_sb_append_buf(sb, buf, size) da_append_many(sb, buf, size)

    // append a null-terminated string to a string builder
    #define flux_sb_append_cstr(sb, cstr)\
        do {\
            const char *s = (cstr);\
            size_t n = strlen(s);\
            da_append_many(sb, s, n);\
        } while (0)

    // appends a single null character to string builder, for use as c string
    #define flux_sb_append_null(sb) da_append_many(sb, "", 1)

    #define flux_sb_free(sb) da_free(sb)
// END STRING BUILDER

// FS
    da_define(Flux_File_Paths, const char *);

    typedef enum {
        FLUX_FILE_REGULAR = 0,
        FLUX_FILE_DIRECTORY,
        FLUX_FILE_SYMLINK,
        FLUX_FILE_OTHER,
    } Flux_File_Type;

    bool flux_mkdir_if_not_exists(const char *path);
    bool flux_copy_file(const char *src_path, const char *dst_path);
    bool flux_copy_directory_recursively(const char *src_path, const char *dst_path);
    bool flux_read_entire_dir(const char *parent, Flux_File_Paths *children);
    bool flux_write_entire_file(const char *path, const void *data, size_t size);
    Flux_File_Type flux_get_file_type(const char *path);
    bool flux_delete_file(const char *path);
    bool flux_read_entire_file(const char *path, Flux_String_Builder *sb);
    Flux_Fd flux_fd_open_for_read(const char *path);
    Flux_Fd flux_fd_open_for_write(const char *path);
    void flux_fd_close(Flux_Fd fd);
// END FS

// PROCS
    da_define(Flux_Procs, Flux_Proc);

    bool flux_proc_wait(Flux_Proc proc);
    bool flux_procs_wait(Flux_Procs procs);
    bool flux_procs_wait_and_reset(Flux_Procs *procs);
    // append a new process to procs array and if procs.count reaches max_procs_count call flux_procs_wait_and_reset() on it
    bool flux_procs_append_with_flush(Flux_Procs *procs, Flux_Proc proc, size_t max_procs_count);
// END PROCS

// CMDS
    da_define(Flux_Cmd, const char *);

    typedef struct {
        Flux_Fd *fdin;
        Flux_Fd *fdout;
        Flux_Fd *fderr;
    } Flux_Cmd_Redirect;

    // Render a string representation of a command into a string builder, not null terminated by default
    void flux_cmd_render(Flux_Cmd cmd, Flux_String_Builder *render);

    #define flux_cmd_append(cmd, ...) da_append_many(cmd, ((const char*[]){__VA_ARGS__}),\
                       (sizeof((const char*[]){__VA_ARGS__})/sizeof(const char*)))

    #define flux_cmd_extend(cmd, other_cmd) da_append_many(cmd, (other_cmd)->items, (other_cmd)->count)

    #define flux_cmd_free(cmd) FLUX_FREE(cmd.items)

    // Run command asynchronously
    #define flux_cmd_run_async(cmd) flux_cmd_run_async_redirect(cmd, (Flux_Cmd_Redirect) {0})
    // NOTE: flux_cmd_run_async_and_reset() is just like flux_cmd_run_async() except it also resets cmd.count to 0
    Flux_Proc flux_cmd_run_async_and_reset(Flux_Cmd *cmd);
    // Run redirected command asynchronously
    Flux_Proc flux_cmd_run_async_redirect(Flux_Cmd cmd, Flux_Cmd_Redirect redirect);
    // Run redirected command asynchronously and set cmd.count to 0 and close all the opened files
    Flux_Proc flux_cmd_run_async_redirect_and_reset(Flux_Cmd *cmd, Flux_Cmd_Redirect redirect);

    // Run command synchronously
    bool flux_cmd_run_sync(Flux_Cmd cmd);
    // NOTE: flux_cmd_run_sync_and_reset() is just like flux_cmd_run_sync() except it also resets cmd.count to 0
    bool flux_cmd_run_sync_and_reset(Flux_Cmd *cmd);
    // Run redirected command synchronously
    bool flux_cmd_run_sync_redirect(Flux_Cmd cmd, Flux_Cmd_Redirect redirect);
    // Run redirected command synchronously and set cmd.count to 0 and close all the opened files
    bool flux_cmd_run_sync_redirect_and_reset(Flux_Cmd *cmd, Flux_Cmd_Redirect redirect);

// END CMDS

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