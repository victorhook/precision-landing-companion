#include "log.h"
#include <stdarg.h>  // ✅ Required for variable arguments

extern Telemetry telemetry;


void telem_printf(log_level_t level, const char *format, ...)
{
    // ✅ Format the message
    va_list args;
    va_start(args, format);
    char buf[50];
    vsnprintf(buf, TELEMTRY_LOG_MSG_MAX_SIZE, format, args);
    telemetry.sendLogMsg(level, buf);
    printf("%s", buf);
    va_end(args);
}
