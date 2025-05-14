#ifndef INCLUDE_PLATFORM_H
#define INCLUDE_PLATFORM_H

#include "common.h"

#define DYNAMIC_ARRAY
#define STRING_BUILDER
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

// FILE SYSTEM
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
// END FILE SYSTEM

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

#endif // INCLUDE_PLATFORM_H

#ifdef PLATFORM_IMPLEMENTATION



#endif // PLATFORM_IMPLEMENTATION

#ifndef PLATFORM_H_FLUX_PREFIX_GUARD
#define PLATFORM_H_FLUX_PREFIX_GUARD
    #ifndef PLATFORM_H_FLUX_PREFIX



    #endif // PLATFORM_H_FLUX_PREFIX
#endif // PLATFORM_H_FLUX_PREFIX_GUARD