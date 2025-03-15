#include "log.h"
#include <stdarg.h>  // ✅ Required for variable arguments

extern Telemetry telemetry;


void telem_printf(const log_level_t level, const log_group_t group, const char *format, ...)
{
    // ✅ Format the message
    va_list args;
    va_start(args, format);
    char buf[100];
    vsnprintf(buf, TELEMTRY_LOG_MSG_MAX_SIZE, format, args);
    telemetry.sendLogMsg(level, group, buf);
    printf("%s", buf);
    va_end(args);
}
