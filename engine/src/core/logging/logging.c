#include "logging.h"

#include "common/common.h"

static struct Flux_Log {

};

void _flux_log(i8 *file, i32 line, Flux_Log_Level lvl, i8 *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    // compute size needed with vsnprintf() into zero-size buffer
    size_t len = vsnprintf(tmpbuf, 0, fmt, args);
    
    uintptr_t i = __atomic_fetch_add();
}

/* OLD LOG METHOD
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
*/