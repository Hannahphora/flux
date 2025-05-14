#ifndef INCLUDE_HLP_H
#define INCLUDE_HLP_H

#include "src/common/common.h"

#ifndef HLP_TEMP_CAPACITY
    #define HLP_TEMP_CAPACITY (8*1024*1024)
#endif

char *hlp_temp_strdup(const char *cstr);
void *hlp_temp_alloc(size_t size);
char *hlp_temp_sprintf(const char *format, ...);
void hlp_temp_reset(void);
size_t hlp_temp_save(void);
void hlp_temp_rewind(size_t checkpoint);

const char *hlp_path_name(const char *path);
bool hlp_rename(const char *old_path, const char *new_path);
int hlp_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count);
int hlp_needs_rebuild1(const char *output_path, const char *input_path);
int hlp_file_exists(const char *file_path);
const char *hlp_get_current_dir_temp(void);
bool hlp_set_current_dir(const char *path);

#ifndef HLP_AUTO_REBUILD
#  if defined(_WIN32)
#    if defined(__GNUC__)
#       define HLP_AUTO_REBUILD(binary_path, source_path) "gcc", "-o", binary_path, source_path
#    elif defined(__clang__)
#       define HLP_AUTO_REBUILD(binary_path, source_path) "clang", "-o", binary_path, source_path
#    elif defined(_MSC_VER)
#       define HLP_AUTO_REBUILD(binary_path, source_path) "cl.exe", hlp_temp_sprintf("/Fe:%s", (binary_path)), source_path
#    endif
#  else
#    define HLP_AUTO_REBUILD(binary_path, source_path) "cc", "-o", binary_path, source_path
#  endif
#endif // HLP_AUTO_REBUILD

void _hlp_auto_rebuild(int argc, char **argv, const char *source_path, ...);
#define HLP_ENABLE_AUTO_REBUILD(argc, argv) _hlp_auto_rebuild(argc, argv, __FILE__, NULL)
#define HLP_ENABLE_AUTO_REBUILD_PLUS(argc, argv, ...) _hlp_auto_rebuild(argc, argv, __FILE__, __VA_ARGS__, NULL);

typedef struct {
    size_t count;
    const char *data;
} Hlp_String_View;

const char *hlp_temp_sv_to_cstr(Hlp_String_View sv);

Hlp_String_View hlp_sv_chop_by_delim(Hlp_String_View *sv, char delim);
Hlp_String_View hlp_sv_chop_left(Hlp_String_View *sv, size_t n);
Hlp_String_View hlp_sv_trim(Hlp_String_View sv);
Hlp_String_View hlp_sv_trim_left(Hlp_String_View sv);
Hlp_String_View hlp_sv_trim_right(Hlp_String_View sv);
bool hlp_sv_eq(Hlp_String_View a, Hlp_String_View b);
bool hlp_sv_end_with(Hlp_String_View sv, const char *cstr);
bool hlp_sv_starts_with(Hlp_String_View sv, Hlp_String_View expected_prefix);
Hlp_String_View hlp_sv_from_cstr(const char *cstr);
Hlp_String_View hlp_sv_from_parts(const char *data, size_t count);
// hlp_sb_to_sv() enables you to just view Hlp_String_Builder as Hlp_String_View
#define hlp_sb_to_sv(sb) hlp_sv_from_parts((sb).items, (sb).count)

// printf macros for String_View
// USAGE:
//   String_View name = ...;
//   printf("Name: "SV_Fmt"\n", SV_Arg(name));
#ifndef SV_Fmt
#define SV_Fmt "%.*s"
#endif // SV_Fmt
#ifndef SV_Arg
#define SV_Arg(sv) (int) (sv).count, (sv).data
#endif // SV_Arg

#endif // INCLUDE_HLP_H

#ifdef HLP_IMPLEMENTATION

// Any messages with the level below hlp_minimal_log_level will be be suppressed
Hlp_Log_Level hlp_minimal_log_level = HLP_INFO;

#ifdef _WIN32

#ifndef HLP_WIN32_ERR_MSG_SIZE
#define HLP_WIN32_ERR_MSG_SIZE (4 * 1024)
#endif // HLP_WIN32_ERR_MSG_SIZE

char *hlp_win32_error_message(DWORD err) {
    static char win32ErrMsg[HLP_WIN32_ERR_MSG_SIZE] = {0};
    DWORD errMsgSize = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, err, LANG_USER_DEFAULT, win32ErrMsg, HLP_WIN32_ERR_MSG_SIZE, NULL);

    if (errMsgSize == 0) {
        if (GetLastError() != ERROR_MR_MID_NOT_FOUND) {
            if (sprintf(win32ErrMsg, "Could not get error message for 0x%lX", err) > 0) {
                return (char *)&win32ErrMsg;
            } else {
                return NULL;
            }
        } else {
            if (sprintf(win32ErrMsg, "Invalid Windows Error code (0x%lX)", err) > 0) {
                return (char *)&win32ErrMsg;
            } else {
                return NULL;
            }
        }
    }

    while (errMsgSize > 1 && isspace(win32ErrMsg[errMsgSize - 1])) {
        win32ErrMsg[--errMsgSize] = '\0';
    }

    return win32ErrMsg;
}

#endif // _WIN32

void _hlp_auto_rebuild(int argc, char **argv, const char *source_path, ...)
{
    const char *binary_path = hlp_shift(argv, argc);
#ifdef _WIN32
    if (!hlp_sv_end_with(hlp_sv_from_cstr(binary_path), ".exe")) {
        binary_path = hlp_temp_sprintf("%s.exe", binary_path);
    }
#endif

    Hlp_File_Paths source_paths = {0};
    da_append(&source_paths, source_path);
    va_list args;
    va_start(args, source_path);
    for (;;) {
        const char *path = va_arg(args, const char*);
        if (path == NULL) break;
        da_append(&source_paths, path);
    }
    va_end(args);

    int rebuild_is_needed = hlp_needs_rebuild(binary_path, source_paths.items, source_paths.count);
    if (rebuild_is_needed < 0) exit(1); // error occurred
    if (!rebuild_is_needed) {
        HLP_FREE(source_paths.items);
        return;
    }

    Hlp_Cmd cmd = {0};

    const char *old_binary_path = hlp_temp_sprintf("%s.old", binary_path);

    if (!hlp_rename(binary_path, old_binary_path)) exit(1);
    hlp_cmd_append(&cmd, HLP_AUTO_REBUILD(binary_path, source_path));
    if (!hlp_cmd_run_sync_and_reset(&cmd)) {
        hlp_rename(old_binary_path, binary_path);
        exit(1);
    }

    // cant delete binary of a running proc on windows >~<
#ifndef _WIN32
    hlp_delete_file(old_binary_path);
#endif

    hlp_cmd_append(&cmd, binary_path);
    da_append_many(&cmd, argv, argc);
    if (!hlp_cmd_run_sync_and_reset(&cmd)) exit(1);
    exit(0);
}

static size_t hlp_temp_size = 0;
static char hlp_temp[HLP_TEMP_CAPACITY] = {0};

bool hlp_mkdir_if_not_exists(const char *path)
{
#ifdef _WIN32
    int result = mkdir(path);
#else
    int result = mkdir(path, 0755);
#endif
    if (result < 0) {
        if (errno == EEXIST) {
            hlp_log(HLP_INFO, "directory `%s` already exists", path);
            return true;
        }
        hlp_log(HLP_ERROR, "could not create directory `%s`: %s", path, strerror(errno));
        return false;
    }

    hlp_log(HLP_INFO, "created directory `%s`", path);
    return true;
}

bool hlp_copy_file(const char *src_path, const char *dst_path)
{
    hlp_log(HLP_INFO, "copying %s -> %s", src_path, dst_path);
#ifdef _WIN32
    if (!CopyFile(src_path, dst_path, FALSE)) {
        hlp_log(HLP_ERROR, "Could not copy file: %s", hlp_win32_error_message(GetLastError()));
        return false;
    }
    return true;
#else
    int src_fd = -1;
    int dst_fd = -1;
    size_t buf_size = 32*1024;
    char *buf = HLP_REALLOC(NULL, buf_size);
    HLP_ASSERT(buf != NULL && "Buy more RAM lol!!");
    bool result = true;

    src_fd = open(src_path, O_RDONLY);
    if (src_fd < 0) {
        hlp_log(HLP_ERROR, "Could not open file %s: %s", src_path, strerror(errno));
        hlp_return_defer(false);
    }

    struct stat src_stat;
    if (fstat(src_fd, &src_stat) < 0) {
        hlp_log(HLP_ERROR, "Could not get mode of file %s: %s", src_path, strerror(errno));
        hlp_return_defer(false);
    }

    dst_fd = open(dst_path, O_CREAT | O_TRUNC | O_WRONLY, src_stat.st_mode);
    if (dst_fd < 0) {
        hlp_log(HLP_ERROR, "Could not create file %s: %s", dst_path, strerror(errno));
        hlp_return_defer(false);
    }

    for (;;) {
        ssize_t n = read(src_fd, buf, buf_size);
        if (n == 0) break;
        if (n < 0) {
            hlp_log(HLP_ERROR, "Could not read from file %s: %s", src_path, strerror(errno));
            hlp_return_defer(false);
        }
        char *buf2 = buf;
        while (n > 0) {
            ssize_t m = write(dst_fd, buf2, n);
            if (m < 0) {
                hlp_log(HLP_ERROR, "Could not write to file %s: %s", dst_path, strerror(errno));
                hlp_return_defer(false);
            }
            n    -= m;
            buf2 += m;
        }
    }

defer:
    HLP_FREE(buf);
    close(src_fd);
    close(dst_fd);
    return result;
#endif
}

void hlp_cmd_render(Hlp_Cmd cmd, Hlp_String_Builder *render)
{
    for (size_t i = 0; i < cmd.count; ++i) {
        const char *arg = cmd.items[i];
        if (arg == NULL) break;
        if (i > 0) hlp_sb_append_cstr(render, " ");
        if (!strchr(arg, ' ')) {
            hlp_sb_append_cstr(render, arg);
        } else {
            da_append(render, '\'');
            hlp_sb_append_cstr(render, arg);
            da_append(render, '\'');
        }
    }
}

Hlp_Proc hlp_cmd_run_async_redirect(Hlp_Cmd cmd, Hlp_Cmd_Redirect redirect)
{
    if (cmd.count < 1) {
        hlp_log(HLP_ERROR, "Could not run empty command");
        return HLP_INVALID_PROC;
    }

    Hlp_String_Builder sb = {0};
    hlp_cmd_render(cmd, &sb);
    hlp_sb_append_null(&sb);
    hlp_log(HLP_INFO, "CMD: %s", sb.items);
    hlp_sb_free(sb);
    memset(&sb, 0, sizeof(sb));

#ifdef _WIN32
    // https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);
    // NOTE: theoretically setting NULL to std handles should not be a problem
    // https://docs.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN#attachdetach-behavior
    // TODO: check for errors in GetStdHandle
    siStartInfo.hStdError = redirect.fderr ? *redirect.fderr : GetStdHandle(STD_ERROR_HANDLE);
    siStartInfo.hStdOutput = redirect.fdout ? *redirect.fdout : GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput = redirect.fdin ? *redirect.fdin : GetStdHandle(STD_INPUT_HANDLE);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    hlp_cmd_render(cmd, &sb);
    hlp_sb_append_null(&sb);
    BOOL bSuccess = CreateProcessA(NULL, sb.items, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
    hlp_sb_free(sb);

    if (!bSuccess) {
        hlp_log(HLP_ERROR, "Could not create child process for %s: %s", cmd.items[0], hlp_win32_error_message(GetLastError()));
        return HLP_INVALID_PROC;
    }

    CloseHandle(piProcInfo.hThread);

    return piProcInfo.hProcess;
#else
    pid_t cpid = fork();
    if (cpid < 0) {
        hlp_log(HLP_ERROR, "Could not fork child process: %s", strerror(errno));
        return HLP_INVALID_PROC;
    }

    if (cpid == 0) {
        if (redirect.fdin) {
            if (dup2(*redirect.fdin, STDIN_FILENO) < 0) {
                hlp_log(HLP_ERROR, "Could not setup stdin for child process: %s", strerror(errno));
                exit(1);
            }
        }

        if (redirect.fdout) {
            if (dup2(*redirect.fdout, STDOUT_FILENO) < 0) {
                hlp_log(HLP_ERROR, "Could not setup stdout for child process: %s", strerror(errno));
                exit(1);
            }
        }

        if (redirect.fderr) {
            if (dup2(*redirect.fderr, STDERR_FILENO) < 0) {
                hlp_log(HLP_ERROR, "Could not setup stderr for child process: %s", strerror(errno));
                exit(1);
            }
        }

        // NOTE: This leaks a bit of memory in the child process.
        // But do we actually care? It's a one off leak anyway...
        Hlp_Cmd cmd_null = {0};
        da_append_many(&cmd_null, cmd.items, cmd.count);
        hlp_cmd_append(&cmd_null, NULL);

        if (execvp(cmd.items[0], (char * const*) cmd_null.items) < 0) {
            hlp_log(HLP_ERROR, "Could not exec child process for %s: %s", cmd.items[0], strerror(errno));
            exit(1);
        }
        HLP_UNREACHABLE("hlp_cmd_run_async_redirect");
    }

    return cpid;
#endif
}

Hlp_Proc hlp_cmd_run_async_and_reset(Hlp_Cmd *cmd)
{
    Hlp_Proc proc = hlp_cmd_run_async(*cmd);
    cmd->count = 0;
    return proc;
}

Hlp_Proc hlp_cmd_run_async_redirect_and_reset(Hlp_Cmd *cmd, Hlp_Cmd_Redirect redirect)
{
    Hlp_Proc proc = hlp_cmd_run_async_redirect(*cmd, redirect);
    cmd->count = 0;
    if (redirect.fdin) {
        hlp_fd_close(*redirect.fdin);
        *redirect.fdin = HLP_INVALID_FD;
    }
    if (redirect.fdout) {
        hlp_fd_close(*redirect.fdout);
        *redirect.fdout = HLP_INVALID_FD;
    }
    if (redirect.fderr) {
        hlp_fd_close(*redirect.fderr);
        *redirect.fderr = HLP_INVALID_FD;
    }
    return proc;
}

Hlp_Fd hlp_fd_open_for_read(const char *path)
{
#ifndef _WIN32
    Hlp_Fd result = open(path, O_RDONLY);
    if (result < 0) {
        hlp_log(HLP_ERROR, "Could not open file %s: %s", path, strerror(errno));
        return HLP_INVALID_FD;
    }
    return result;
#else
    // https://docs.microsoft.com/en-us/windows/win32/fileio/opening-a-file-for-reading-or-writing
    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;

    Hlp_Fd result = CreateFile(
                    path,
                    GENERIC_READ,
                    0,
                    &saAttr,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_READONLY,
                    NULL);

    if (result == INVALID_HANDLE_VALUE) {
        hlp_log(HLP_ERROR, "Could not open file %s: %s", path, hlp_win32_error_message(GetLastError()));
        return HLP_INVALID_FD;
    }

    return result;
#endif // _WIN32
}

Hlp_Fd hlp_fd_open_for_write(const char *path)
{
#ifndef _WIN32
    Hlp_Fd result = open(path,
                     O_WRONLY | O_CREAT | O_TRUNC,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (result < 0) {
        hlp_log(HLP_ERROR, "could not open file %s: %s", path, strerror(errno));
        return HLP_INVALID_FD;
    }
    return result;
#else
    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;

    Hlp_Fd result = CreateFile(
                    path,                            // name of the write
                    GENERIC_WRITE,                   // open for writing
                    0,                               // do not share
                    &saAttr,                         // default security
                    CREATE_ALWAYS,                   // create always
                    FILE_ATTRIBUTE_NORMAL,           // normal file
                    NULL                             // no attr. template
                );

    if (result == INVALID_HANDLE_VALUE) {
        hlp_log(HLP_ERROR, "Could not open file %s: %s", path, hlp_win32_error_message(GetLastError()));
        return HLP_INVALID_FD;
    }

    return result;
#endif // _WIN32
}

void hlp_fd_close(Hlp_Fd fd)
{
#ifdef _WIN32
    CloseHandle(fd);
#else
    close(fd);
#endif // _WIN32
}

bool hlp_procs_wait(Hlp_Procs procs)
{
    bool success = true;
    for (size_t i = 0; i < procs.count; ++i) {
        success = hlp_proc_wait(procs.items[i]) && success;
    }
    return success;
}

bool hlp_procs_wait_and_reset(Hlp_Procs *procs)
{
    bool success = hlp_procs_wait(*procs);
    procs->count = 0;
    return success;
}

bool hlp_proc_wait(Hlp_Proc proc)
{
    if (proc == HLP_INVALID_PROC) return false;

#ifdef _WIN32
    DWORD result = WaitForSingleObject(
                       proc,    // HANDLE hHandle,
                       INFINITE // DWORD  dwMilliseconds
                   );

    if (result == WAIT_FAILED) {
        hlp_log(HLP_ERROR, "could not wait on child process: %s", hlp_win32_error_message(GetLastError()));
        return false;
    }

    DWORD exit_status;
    if (!GetExitCodeProcess(proc, &exit_status)) {
        hlp_log(HLP_ERROR, "could not get process exit code: %s", hlp_win32_error_message(GetLastError()));
        return false;
    }

    if (exit_status != 0) {
        hlp_log(HLP_ERROR, "command exited with exit code %lu", exit_status);
        return false;
    }

    CloseHandle(proc);

    return true;
#else
    for (;;) {
        int wstatus = 0;
        if (waitpid(proc, &wstatus, 0) < 0) {
            hlp_log(HLP_ERROR, "could not wait on command (pid %d): %s", proc, strerror(errno));
            return false;
        }

        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            if (exit_status != 0) {
                hlp_log(HLP_ERROR, "command exited with exit code %d", exit_status);
                return false;
            }

            break;
        }

        if (WIFSIGNALED(wstatus)) {
            hlp_log(HLP_ERROR, "command process was terminated by signal %d", WTERMSIG(wstatus));
            return false;
        }
    }

    return true;
#endif
}

bool hlp_procs_append_with_flush(Hlp_Procs *procs, Hlp_Proc proc, size_t max_procs_count)
{
    da_append(procs, proc);

    if (procs->count >= max_procs_count) {
        if (!hlp_procs_wait_and_reset(procs)) return false;
    }

    return true;
}

bool hlp_cmd_run_sync_redirect(Hlp_Cmd cmd, Hlp_Cmd_Redirect redirect)
{
    Hlp_Proc p = hlp_cmd_run_async_redirect(cmd, redirect);
    if (p == HLP_INVALID_PROC) return false;
    return hlp_proc_wait(p);
}

bool hlp_cmd_run_sync(Hlp_Cmd cmd)
{
    Hlp_Proc p = hlp_cmd_run_async(cmd);
    if (p == HLP_INVALID_PROC) return false;
    return hlp_proc_wait(p);
}

bool hlp_cmd_run_sync_and_reset(Hlp_Cmd *cmd)
{
    bool p = hlp_cmd_run_sync(*cmd);
    cmd->count = 0;
    return p;
}

bool hlp_cmd_run_sync_redirect_and_reset(Hlp_Cmd *cmd, Hlp_Cmd_Redirect redirect)
{
    bool p = hlp_cmd_run_sync_redirect(*cmd, redirect);
    cmd->count = 0;
    if (redirect.fdin) {
        hlp_fd_close(*redirect.fdin);
        *redirect.fdin = HLP_INVALID_FD;
    }
    if (redirect.fdout) {
        hlp_fd_close(*redirect.fdout);
        *redirect.fdout = HLP_INVALID_FD;
    }
    if (redirect.fderr) {
        hlp_fd_close(*redirect.fderr);
        *redirect.fderr = HLP_INVALID_FD;
    }
    return p;
}

void hlp_log(Hlp_Log_Level level, const char *fmt, ...)
{
    if (level < hlp_minimal_log_level) return;

    switch (level) {
    case HLP_INFO:
        fprintf(stderr, "[INFO] ");
        break;
    case HLP_WARNING:
        fprintf(stderr, "[WARNING] ");
        break;
    case HLP_ERROR:
        fprintf(stderr, "[ERROR] ");
        break;
    case HLP_NO_LOGS: return;
    default:
        HLP_UNREACHABLE("hlp_log");
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

bool hlp_read_entire_dir(const char *parent, Hlp_File_Paths *children)
{
    bool result = true;
    DIR *dir = NULL;

    dir = opendir(parent);
    if (dir == NULL) {
        #ifdef _WIN32
        hlp_log(HLP_ERROR, "Could not open directory %s: %s", parent, hlp_win32_error_message(GetLastError()));
        #else
        hlp_log(HLP_ERROR, "Could not open directory %s: %s", parent, strerror(errno));
        #endif // _WIN32
        hlp_return_defer(false);
    }

    errno = 0;
    struct dirent *ent = readdir(dir);
    while (ent != NULL) {
        da_append(children, hlp_temp_strdup(ent->d_name));
        ent = readdir(dir);
    }

    if (errno != 0) {
        #ifdef _WIN32
        hlp_log(HLP_ERROR, "Could not read directory %s: %s", parent, hlp_win32_error_message(GetLastError()));
        #else
        hlp_log(HLP_ERROR, "Could not read directory %s: %s", parent, strerror(errno));
        #endif // _WIN32
        hlp_return_defer(false);
    }

defer:
    if (dir) closedir(dir);
    return result;
}

bool hlp_write_entire_file(const char *path, const void *data, size_t size)
{
    bool result = true;

    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        hlp_log(HLP_ERROR, "Could not open file %s for writing: %s\n", path, strerror(errno));
        hlp_return_defer(false);
    }

    //           len
    //           v
    // aaaaaaaaaa
    //     ^
    //     data

    const char *buf = data;
    while (size > 0) {
        size_t n = fwrite(buf, 1, size, f);
        if (ferror(f)) {
            hlp_log(HLP_ERROR, "Could not write into file %s: %s\n", path, strerror(errno));
            hlp_return_defer(false);
        }
        size -= n;
        buf  += n;
    }

defer:
    if (f) fclose(f);
    return result;
}

Hlp_File_Type hlp_get_file_type(const char *path)
{
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        hlp_log(HLP_ERROR, "Could not get file attributes of %s: %s", path, hlp_win32_error_message(GetLastError()));
        return -1;
    }

    if (attr & FILE_ATTRIBUTE_DIRECTORY) return HLP_FILE_DIRECTORY;
    // TODO: detect symlinks on Windows (whatever that means on Windows anyway)
    return HLP_FILE_REGULAR;
#else // _WIN32
    struct stat statbuf;
    if (stat(path, &statbuf) < 0) {
        hlp_log(HLP_ERROR, "Could not get stat of %s: %s", path, strerror(errno));
        return -1;
    }

    if (S_ISREG(statbuf.st_mode)) return HLP_FILE_REGULAR;
    if (S_ISDIR(statbuf.st_mode)) return HLP_FILE_DIRECTORY;
    if (S_ISLNK(statbuf.st_mode)) return HLP_FILE_SYMLINK;
    return HLP_FILE_OTHER;
#endif // _WIN32
}

bool hlp_delete_file(const char *path)
{
    hlp_log(HLP_INFO, "deleting %s", path);
#ifdef _WIN32
    if (!DeleteFileA(path)) {
        hlp_log(HLP_ERROR, "Could not delete file %s: %s", path, hlp_win32_error_message(GetLastError()));
        return false;
    }
    return true;
#else
    if (remove(path) < 0) {
        hlp_log(HLP_ERROR, "Could not delete file %s: %s", path, strerror(errno));
        return false;
    }
    return true;
#endif // _WIN32
}

#ifdef _WIN32
bool hlp_win_delete_current_binary(const char *path)
{
    hlp_log(HLP_INFO, "deleting %s", path);

    da_define(Wide_Str, WCHAR);
    Wide_Str p = {0};
    // da_append_many(&p, path);
    // hlp_sb_append_cstr(&p, path);
    // hlp_sb_append_null(&p);

    SHFILEOPSTRUCTW file_op = {0};
    file_op.hwnd = NULL;
    file_op.wFunc = FO_DELETE;
    file_op.pFrom = p.items;
    file_op.pTo = NULL;
    file_op.fFlags = FOF_NOCONFIRMATION
                  | FOF_NOERRORUI            // no UI on errors
                  | FOF_SILENT;              // don’t show progress dialog

    // 4. Perform the operation
    int result = SHFileOperationW(&file_op);
    if (result != 0) {
        wprintf(L"Failed to delete the file: error code %d\n", result);
    }

    return true;
}
#endif // _WIN32

bool hlp_copy_directory_recursively(const char *src_path, const char *dst_path)
{
    bool result = true;
    Hlp_File_Paths children = {0};
    Hlp_String_Builder src_sb = {0};
    Hlp_String_Builder dst_sb = {0};
    size_t temp_checkpoint = hlp_temp_save();

    Hlp_File_Type type = hlp_get_file_type(src_path);
    if (type < 0) return false;

    switch (type) {
        case HLP_FILE_DIRECTORY: {
            if (!hlp_mkdir_if_not_exists(dst_path)) hlp_return_defer(false);
            if (!hlp_read_entire_dir(src_path, &children)) hlp_return_defer(false);

            for (size_t i = 0; i < children.count; ++i) {
                if (strcmp(children.items[i], ".") == 0) continue;
                if (strcmp(children.items[i], "..") == 0) continue;

                src_sb.count = 0;
                hlp_sb_append_cstr(&src_sb, src_path);
                hlp_sb_append_cstr(&src_sb, "/");
                hlp_sb_append_cstr(&src_sb, children.items[i]);
                hlp_sb_append_null(&src_sb);

                dst_sb.count = 0;
                hlp_sb_append_cstr(&dst_sb, dst_path);
                hlp_sb_append_cstr(&dst_sb, "/");
                hlp_sb_append_cstr(&dst_sb, children.items[i]);
                hlp_sb_append_null(&dst_sb);

                if (!hlp_copy_directory_recursively(src_sb.items, dst_sb.items)) {
                    hlp_return_defer(false);
                }
            }
        } break;

        case HLP_FILE_REGULAR: {
            if (!hlp_copy_file(src_path, dst_path)) {
                hlp_return_defer(false);
            }
        } break;

        case HLP_FILE_SYMLINK: {
            hlp_log(HLP_WARNING, "TODO: Copying symlinks is not supported yet");
        } break;

        case HLP_FILE_OTHER: {
            hlp_log(HLP_ERROR, "Unsupported type of file %s", src_path);
            hlp_return_defer(false);
        } break;

        default: HLP_UNREACHABLE("hlp_copy_directory_recursively");
    }

defer:
    hlp_temp_rewind(temp_checkpoint);
    da_free(src_sb);
    da_free(dst_sb);
    da_free(children);
    return result;
}

char *hlp_temp_strdup(const char *cstr)
{
    size_t n = strlen(cstr);
    char *result = hlp_temp_alloc(n + 1);
    HLP_ASSERT(result != NULL && "Increase HLP_TEMP_CAPACITY");
    memcpy(result, cstr, n);
    result[n] = '\0';
    return result;
}

void *hlp_temp_alloc(size_t size)
{
    if (hlp_temp_size + size > HLP_TEMP_CAPACITY) return NULL;
    void *result = &hlp_temp[hlp_temp_size];
    hlp_temp_size += size;
    return result;
}

char *hlp_temp_sprintf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int n = vsnprintf(NULL, 0, format, args);
    va_end(args);

    HLP_ASSERT(n >= 0);
    char *result = hlp_temp_alloc(n + 1);
    HLP_ASSERT(result != NULL && "Extend the size of the temporary allocator");
    // TODO: use proper arenas for the temporary allocator;
    va_start(args, format);
    vsnprintf(result, n + 1, format, args);
    va_end(args);

    return result;
}

void hlp_temp_reset(void)
{
    hlp_temp_size = 0;
}

size_t hlp_temp_save(void)
{
    return hlp_temp_size;
}

void hlp_temp_rewind(size_t checkpoint)
{
    hlp_temp_size = checkpoint;
}

const char *hlp_temp_sv_to_cstr(Hlp_String_View sv)
{
    char *result = hlp_temp_alloc(sv.count + 1);
    HLP_ASSERT(result != NULL && "Extend the size of the temporary allocator");
    memcpy(result, sv.data, sv.count);
    result[sv.count] = '\0';
    return result;
}

int hlp_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count)
{
#ifdef _WIN32
    BOOL bSuccess;

    HANDLE output_path_fd = CreateFile(output_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if (output_path_fd == INVALID_HANDLE_VALUE) {
        // NOTE: if output does not exist it 100% must be rebuilt
        if (GetLastError() == ERROR_FILE_NOT_FOUND) return 1;
        hlp_log(HLP_ERROR, "Could not open file %s: %s", output_path, hlp_win32_error_message(GetLastError()));
        return -1;
    }
    FILETIME output_path_time;
    bSuccess = GetFileTime(output_path_fd, NULL, NULL, &output_path_time);
    CloseHandle(output_path_fd);
    if (!bSuccess) {
        hlp_log(HLP_ERROR, "Could not get time of %s: %s", output_path, hlp_win32_error_message(GetLastError()));
        return -1;
    }

    for (size_t i = 0; i < input_paths_count; ++i) {
        const char *input_path = input_paths[i];
        HANDLE input_path_fd = CreateFile(input_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
        if (input_path_fd == INVALID_HANDLE_VALUE) {
            // NOTE: non-existing input is an error cause it is needed for building in the first place
            hlp_log(HLP_ERROR, "Could not open file %s: %s", input_path, hlp_win32_error_message(GetLastError()));
            return -1;
        }
        FILETIME input_path_time;
        bSuccess = GetFileTime(input_path_fd, NULL, NULL, &input_path_time);
        CloseHandle(input_path_fd);
        if (!bSuccess) {
            hlp_log(HLP_ERROR, "Could not get time of %s: %s", input_path, hlp_win32_error_message(GetLastError()));
            return -1;
        }

        // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
        if (CompareFileTime(&input_path_time, &output_path_time) == 1) return 1;
    }

    return 0;
#else
    struct stat statbuf = {0};

    if (stat(output_path, &statbuf) < 0) {
        // NOTE: if output does not exist it 100% must be rebuilt
        if (errno == ENOENT) return 1;
        hlp_log(HLP_ERROR, "could not stat %s: %s", output_path, strerror(errno));
        return -1;
    }
    int output_path_time = statbuf.st_mtime;

    for (size_t i = 0; i < input_paths_count; ++i) {
        const char *input_path = input_paths[i];
        if (stat(input_path, &statbuf) < 0) {
            // NOTE: non-existing input is an error cause it is needed for building in the first place
            hlp_log(HLP_ERROR, "could not stat %s: %s", input_path, strerror(errno));
            return -1;
        }
        int input_path_time = statbuf.st_mtime;
        // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
        if (input_path_time > output_path_time) return 1;
    }

    return 0;
#endif
}

int hlp_needs_rebuild1(const char *output_path, const char *input_path)
{
    return hlp_needs_rebuild(output_path, &input_path, 1);
}

const char *hlp_path_name(const char *path)
{
#ifdef _WIN32
    const char *p1 = strrchr(path, '/');
    const char *p2 = strrchr(path, '\\');
    const char *p = (p1 > p2)? p1 : p2;  // NULL is ignored if the other search is successful
    return p ? p + 1 : path;
#else
    const char *p = strrchr(path, '/');
    return p ? p + 1 : path;
#endif // _WIN32
}

bool hlp_rename(const char *old_path, const char *new_path)
{
    hlp_log(HLP_INFO, "renaming %s -> %s", old_path, new_path);
#ifdef _WIN32
    if (!MoveFileEx(old_path, new_path, MOVEFILE_REPLACE_EXISTING)) {
        hlp_log(HLP_ERROR, "could not rename %s to %s: %s", old_path, new_path, hlp_win32_error_message(GetLastError()));
        return false;
    }
#else
    if (rename(old_path, new_path) < 0) {
        hlp_log(HLP_ERROR, "could not rename %s to %s: %s", old_path, new_path, strerror(errno));
        return false;
    }
#endif // _WIN32
    return true;
}

bool hlp_read_entire_file(const char *path, Hlp_String_Builder *sb)
{
    bool result = true;

    FILE *f = fopen(path, "rb");
    if (f == NULL)                 hlp_return_defer(false);
    if (fseek(f, 0, SEEK_END) < 0) hlp_return_defer(false);
#ifndef _WIN32
    long m = ftell(f);
#else
    long long m = _ftelli64(f);
#endif
    if (m < 0)                     hlp_return_defer(false);
    if (fseek(f, 0, SEEK_SET) < 0) hlp_return_defer(false);

    size_t new_count = sb->count + m;
    if (new_count > sb->capacity) {
        sb->items = HLP_REALLOC(sb->items, new_count);
        HLP_ASSERT(sb->items != NULL && "Buy more RAM lool!!");
        sb->capacity = new_count;
    }

    fread(sb->items + sb->count, m, 1, f);
    if (ferror(f)) {
        // TODO: Afaik, ferror does not set errno. So the error reporting in defer is not correct in this case.
        hlp_return_defer(false);
    }
    sb->count = new_count;

defer:
    if (!result) hlp_log(HLP_ERROR, "Could not read file %s: %s", path, strerror(errno));
    if (f) fclose(f);
    return result;
}

int hlp_sb_appendf(Hlp_String_Builder *sb, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    int n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    // NOTE: the new_capacity needs to be +1 because of the null terminator.
    // However, further below we increase sb->count by n, not n + 1.
    // This is because we don't want the sb to include the null terminator. The user can always sb_append_null() if they want it
    da_reserve(sb, sb->count + n + 1);
    char *dest = sb->items + sb->count;
    va_start(args, fmt);
    vsnprintf(dest, n+1, fmt, args);
    va_end(args);

    sb->count += n;

    return n;
}

Hlp_String_View hlp_sv_chop_by_delim(Hlp_String_View *sv, char delim)
{
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) {
        i += 1;
    }

    Hlp_String_View result = hlp_sv_from_parts(sv->data, i);

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data  += i + 1;
    } else {
        sv->count -= i;
        sv->data  += i;
    }

    return result;
}

Hlp_String_View hlp_sv_chop_left(Hlp_String_View *sv, size_t n)
{
    if (n > sv->count) {
        n = sv->count;
    }

    Hlp_String_View result = hlp_sv_from_parts(sv->data, n);

    sv->data  += n;
    sv->count -= n;

    return result;
}

Hlp_String_View hlp_sv_from_parts(const char *data, size_t count)
{
    Hlp_String_View sv;
    sv.count = count;
    sv.data = data;
    return sv;
}

Hlp_String_View hlp_sv_trim_left(Hlp_String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[i])) {
        i += 1;
    }

    return hlp_sv_from_parts(sv.data + i, sv.count - i);
}

Hlp_String_View hlp_sv_trim_right(Hlp_String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) {
        i += 1;
    }

    return hlp_sv_from_parts(sv.data, sv.count - i);
}

Hlp_String_View hlp_sv_trim(Hlp_String_View sv)
{
    return hlp_sv_trim_right(hlp_sv_trim_left(sv));
}

Hlp_String_View hlp_sv_from_cstr(const char *cstr)
{
    return hlp_sv_from_parts(cstr, strlen(cstr));
}

bool hlp_sv_eq(Hlp_String_View a, Hlp_String_View b)
{
    if (a.count != b.count) {
        return false;
    } else {
        return memcmp(a.data, b.data, a.count) == 0;
    }
}

bool hlp_sv_end_with(Hlp_String_View sv, const char *cstr)
{
    size_t cstr_count = strlen(cstr);
    if (sv.count >= cstr_count) {
        size_t ending_start = sv.count - cstr_count;
        Hlp_String_View sv_ending = hlp_sv_from_parts(sv.data + ending_start, cstr_count);
        return hlp_sv_eq(sv_ending, hlp_sv_from_cstr(cstr));
    }
    return false;
}


bool hlp_sv_starts_with(Hlp_String_View sv, Hlp_String_View expected_prefix)
{
    if (expected_prefix.count <= sv.count) {
        Hlp_String_View actual_prefix = hlp_sv_from_parts(sv.data, expected_prefix.count);
        return hlp_sv_eq(expected_prefix, actual_prefix);
    }

    return false;
}

// RETURNS:
//  0 - file does not exists
//  1 - file exists
// -1 - error while checking if file exists. The error is logged
int hlp_file_exists(const char *file_path)
{
#if _WIN32
    // TODO: distinguish between "does not exists" and other errors
    DWORD dwAttrib = GetFileAttributesA(file_path);
    return dwAttrib != INVALID_FILE_ATTRIBUTES;
#else
    struct stat statbuf;
    if (stat(file_path, &statbuf) < 0) {
        if (errno == ENOENT) return 0;
        hlp_log(HLP_ERROR, "Could not check if file %s exists: %s", file_path, strerror(errno));
        return -1;
    }
    return 1;
#endif
}

const char *hlp_get_current_dir_temp(void)
{
#ifdef _WIN32
    DWORD nBufferLength = GetCurrentDirectory(0, NULL);
    if (nBufferLength == 0) {
        hlp_log(HLP_ERROR, "could not get current directory: %s", hlp_win32_error_message(GetLastError()));
        return NULL;
    }

    char *buffer = (char*) hlp_temp_alloc(nBufferLength);
    if (GetCurrentDirectory(nBufferLength, buffer) == 0) {
        hlp_log(HLP_ERROR, "could not get current directory: %s", hlp_win32_error_message(GetLastError()));
        return NULL;
    }

    return buffer;
#else
    char *buffer = (char*) hlp_temp_alloc(PATH_MAX);
    if (getcwd(buffer, PATH_MAX) == NULL) {
        hlp_log(HLP_ERROR, "could not get current directory: %s", strerror(errno));
        return NULL;
    }

    return buffer;
#endif // _WIN32
}

bool hlp_set_current_dir(const char *path)
{
#ifdef _WIN32
    if (!SetCurrentDirectory(path)) {
        hlp_log(HLP_ERROR, "could not set current directory to %s: %s", path, hlp_win32_error_message(GetLastError()));
        return false;
    }
    return true;
#else
    if (chdir(path) < 0) {
        hlp_log(HLP_ERROR, "could not set current directory to %s: %s", path, strerror(errno));
        return false;
    }
    return true;
#endif // _WIN32
}

#ifdef _WIN32
struct DIR
{
    HANDLE hFind;
    WIN32_FIND_DATA data;
    struct dirent *dirent;
};

DIR *opendir(const char *dirpath)
{
    HLP_ASSERT(dirpath);

    char buffer[MAX_PATH];
    snprintf(buffer, MAX_PATH, "%s\\*", dirpath);

    DIR *dir = (DIR*)HLP_REALLOC(NULL, sizeof(DIR));
    memset(dir, 0, sizeof(DIR));

    dir->hFind = FindFirstFile(buffer, &dir->data);
    if (dir->hFind == INVALID_HANDLE_VALUE) {
        // TODO: opendir should set errno accordingly on FindFirstFile fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        goto fail;
    }

    return dir;

fail:
    if (dir) {
        HLP_FREE(dir);
    }

    return NULL;
}

struct dirent *readdir(DIR *dirp)
{
    HLP_ASSERT(dirp);

    if (dirp->dirent == NULL) {
        dirp->dirent = (struct dirent*)HLP_REALLOC(NULL, sizeof(struct dirent));
        memset(dirp->dirent, 0, sizeof(struct dirent));
    } else {
        if(!FindNextFile(dirp->hFind, &dirp->data)) {
            if (GetLastError() != ERROR_NO_MORE_FILES) {
                // TODO: readdir should set errno accordingly on FindNextFile fail
                // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
                errno = ENOSYS;
            }

            return NULL;
        }
    }

    memset(dirp->dirent->d_name, 0, sizeof(dirp->dirent->d_name));

    strncpy(
        dirp->dirent->d_name,
        dirp->data.cFileName,
        sizeof(dirp->dirent->d_name) - 1);

    return dirp->dirent;
}

int closedir(DIR *dirp)
{
    HLP_ASSERT(dirp);

    if(!FindClose(dirp->hFind)) {
        // TODO: closedir should set errno accordingly on FindClose fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        return -1;
    }

    if (dirp->dirent) {
        HLP_FREE(dirp->dirent);
    }
    HLP_FREE(dirp);

    return 0;
}
#endif // _WIN32

#endif // HLP_IMPLEMENTATION

#ifndef HLP_USE_PREFIX_GUARD_
#define HLP_USE_PREFIX_GUARD_
    #ifndef HLP_USE_PREFIX
        #define ENABLE_AUTO_REBUILD HLP_ENABLE_AUTO_REBUILD
        #define ENABLE_AUTO_REBUILD_PLUS HLP_ENABLE_AUTO_REBUILD_PLUS
        #define TODO HLP_TODO
        #define UNREACHABLE HLP_UNREACHABLE
        #define UNUSED HLP_UNUSED
        #define ARRAY_LEN HLP_ARRAY_LEN
        #define ARRAY_GET HLP_ARRAY_GET
        #define INFO HLP_INFO
        #define WARNING HLP_WARNING
        #define ERROR HLP_ERROR
        #define NO_LOGS HLP_NO_LOGS
        #define Log_Level Hlp_Log_Level
        #define minimal_log_level hlp_minimal_log_level
        // NOTE: Name log is already defined in math.h, so hlp_log is left as is
        // #define log hlp_log
        #define shift hlp_shift
        #define shift_args hlp_shift_args
        #define File_Paths Hlp_File_Paths
        #define FILE_REGULAR HLP_FILE_REGULAR
        #define FILE_DIRECTORY HLP_FILE_DIRECTORY
        #define FILE_SYMLINK HLP_FILE_SYMLINK
        #define FILE_OTHER HLP_FILE_OTHER
        #define File_Type Hlp_File_Type
        #define mkdir_if_not_exists hlp_mkdir_if_not_exists
        #define copy_file hlp_copy_file
        #define copy_directory_recursively hlp_copy_directory_recursively
        #define read_entire_dir hlp_read_entire_dir
        #define write_entire_file hlp_write_entire_file
        #define get_file_type hlp_get_file_type
        #define delete_file hlp_delete_file
        #define return_defer hlp_return_defer
        #define da_append da_append
        #define da_free da_free
        #define da_append_many da_append_many
        #define da_resize da_resize
        #define da_reserve da_reserve
        #define da_last da_last
        #define da_remove_unordered da_remove_unordered
        #define da_foreach da_foreach
        #define String_Builder Hlp_String_Builder
        #define read_entire_file hlp_read_entire_file
        #define sb_appendf hlp_sb_appendf
        #define sb_append_buf hlp_sb_append_buf
        #define sb_append_cstr hlp_sb_append_cstr
        #define sb_append_null hlp_sb_append_null
        #define sb_free hlp_sb_free
        #define Proc Hlp_Proc
        #define INVALID_PROC HLP_INVALID_PROC
        #define Fd Hlp_Fd
        #define INVALID_FD HLP_INVALID_FD
        #define fd_open_for_read hlp_fd_open_for_read
        #define fd_open_for_write hlp_fd_open_for_write
        #define fd_close hlp_fd_close
        #define Procs Hlp_Procs
        #define proc_wait hlp_proc_wait
        #define procs_wait hlp_procs_wait
        #define procs_wait_and_reset hlp_procs_wait_and_reset
        #define procs_append_with_flush hlp_procs_append_with_flush
        #define Cmd Hlp_Cmd
        #define Cmd_Redirect Hlp_Cmd_Redirect
        #define cmd_render hlp_cmd_render
        #define cmd_append hlp_cmd_append
        #define cmd_extend hlp_cmd_extend
        #define cmd_free hlp_cmd_free
        #define cmd_run_async hlp_cmd_run_async
        #define cmd_run_async_and_reset hlp_cmd_run_async_and_reset
        #define cmd_run_async_redirect hlp_cmd_run_async_redirect
        #define cmd_run_async_redirect_and_reset hlp_cmd_run_async_redirect_and_reset
        #define cmd_run_sync hlp_cmd_run_sync
        #define cmd_run_sync_and_reset hlp_cmd_run_sync_and_reset
        #define cmd_run_sync_redirect hlp_cmd_run_sync_redirect
        #define cmd_run_sync_redirect_and_reset hlp_cmd_run_sync_redirect_and_reset
        #define temp_strdup hlp_temp_strdup
        #define temp_alloc hlp_temp_alloc
        #define temp_sprintf hlp_temp_sprintf
        #define temp_reset hlp_temp_reset
        #define temp_save hlp_temp_save
        #define temp_rewind hlp_temp_rewind
        #define path_name hlp_path_name
        #define rename hlp_rename
        #define needs_rebuild hlp_needs_rebuild
        #define needs_rebuild1 hlp_needs_rebuild1
        #define file_exists hlp_file_exists
        #define get_current_dir_temp hlp_get_current_dir_temp
        #define set_current_dir hlp_set_current_dir
        #define String_View Hlp_String_View
        #define temp_sv_to_cstr hlp_temp_sv_to_cstr
        #define sv_chop_by_delim hlp_sv_chop_by_delim
        #define sv_chop_left hlp_sv_chop_left
        #define sv_trim hlp_sv_trim
        #define sv_trim_left hlp_sv_trim_left
        #define sv_trim_right hlp_sv_trim_right
        #define sv_eq hlp_sv_eq
        #define sv_starts_with hlp_sv_starts_with
        #define sv_end_with hlp_sv_end_with
        #define sv_from_cstr hlp_sv_from_cstr
        #define sv_from_parts hlp_sv_from_parts
        #define sb_to_sv hlp_sb_to_sv
        #define win32_error_message hlp_win32_error_message
    #endif // HLP_USE_PREFIX
#endif // HLP_USE_PREFIX_GUARD_