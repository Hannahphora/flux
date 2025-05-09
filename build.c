#define HLP_IMPLEMENTATION
#include "hlp.h"

#define DA_IMPLEMENTATION
#include "src/common/ds.h"

#define OUT_DIR "out/"
#define SRC_DIR "src/"
#define RES_DIR "res/"

#define COMMON_FLAGS "-std=c99", /*"-save-temps",*/ "-Wextra", "-Wall", "-Wpedantic", "-Werror", "-Wshadow"

#ifdef _WIN32
  #define SANITIZE_FLAGS "" /* sanitize doesnt work on windows */
#else
  #define SANITIZE_FLAGS "-fsanitize=address,leak,undefined"
#endif

#define DEBUG_FLAGS \
    "-O0", \
    "-D_DEBUG", \
    "-g", \
    SANITIZE_FLAGS, \
    "-fno-omit-frame-pointer"
// DEBUG_FLAGS

#define RELEASE_FLAGS "-O3", "-flto", "-D_NDEBUG", "-s"

const bool debug_mode = true;

Cmd cmd = {0};
Procs procs = {0};

bool compile_shaders(const char *src_dir, const char *out_dir);

int main(int argc, char **argv)
{
    ENABLE_AUTO_REBUILD(argc, argv);
    int result = 0;

    if (!mkdir_if_not_exists(OUT_DIR)) return_defer(1);

    if (!compile_shaders(RES_DIR"shaders/", OUT_DIR"shader_cache/")) return_defer(1);

    cmd_append(&cmd, "gcc", COMMON_FLAGS);
    if (debug_mode) {
        if (!mkdir_if_not_exists(OUT_DIR"debug")) return_defer(1);
        cmd_append(&cmd, DEBUG_FLAGS, "-o", OUT_DIR"debug/engine.exe");
    }
    else {
        if (!mkdir_if_not_exists(OUT_DIR"release")) return_defer(1);
        cmd_append(&cmd, RELEASE_FLAGS, "-o", OUT_DIR"release/engine.exe");
    }
    cmd_append(&cmd,
        // includes
        "-Iext/include",
        temp_sprintf("-I%s/Include", getenv("VULKAN_SDK")),
        // inputs
        SRC_DIR"main.c",
        // linking
        temp_sprintf("-L%s/Lib", getenv("VULKAN_SDK")),
#ifdef _WIN32
        "-lvulkan-1",
#else
        "-lvulkan",
#endif
        "-Lext/lib/GLFW",
        "-lglfw3"
    );
    da_append(&procs, cmd_run_async_and_reset(&cmd));

    if (!procs_wait_and_reset(&procs)) return_defer(1);
    
defer:
    da_free(cmd);
    da_free(procs);
    return result;
}

bool compile_shaders(const char *src_dir, const char *out_dir)
{
    bool result = true;
    File_Paths src_children = {0};
    File_Paths shaders = {0};

    if (!read_entire_dir(src_dir, &src_children)) return_defer(false);

    for (size_t i = 0; i < src_children.count; ++i) {
        if (sv_end_with(sv_from_cstr(src_children.items[i]), ".slang")) da_append(&shaders, src_children.items[i]);
        else continue;
    }

    if (!mkdir_if_not_exists(out_dir)) return_defer(false);

    for (size_t i = 0; i < shaders.count; ++i) {
        String_View sv = sv_from_cstr(shaders.items[i]);
        String_View name = sv_chop_by_delim(&sv, '.');

        if (!file_exists(temp_sprintf("%s%s.spv", out_dir, temp_sv_to_cstr(name))) ||
            needs_rebuild1(
                temp_sprintf("%s%s.spv", out_dir, temp_sv_to_cstr(name)),
                temp_sprintf("%s%s", src_dir, shaders.items[i])))
        {
            cmd_append(&cmd,
                "slangc"
            );

            da_append(&procs, cmd_run_async_and_reset(&cmd));
        }
    }
    
defer:
    da_free(src_children);
    da_free(shaders);
    return result;
}