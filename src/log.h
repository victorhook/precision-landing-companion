#ifndef LOG_H
#define LOG_H


#include "telemetry.h"


void telem_printf(log_level_t level, const char *format, ...);

#define debug(format, ...) telem_printf(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)
#define info(format, ...)  telem_printf(LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#define warning(format, ...)  telem_printf(LOG_LEVEL_WARNING, format, ##__VA_ARGS__)
#define error(format, ...) telem_printf(LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

#endif