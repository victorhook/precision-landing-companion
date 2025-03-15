#ifndef LOG_H
#define LOG_H


#include "telemetry.h"


void telem_printf(const log_level_t level, const log_group_t group, const char *format, ...);

#define debug(format, ...)   telem_printf(LOG_LEVEL_DEBUG,   LOG_GROUP_MAIN, format, ##__VA_ARGS__)
#define info(format, ...)    telem_printf(LOG_LEVEL_INFO,    LOG_GROUP_MAIN, format, ##__VA_ARGS__)
#define warning(format, ...) telem_printf(LOG_LEVEL_WARNING, LOG_GROUP_MAIN, format, ##__VA_ARGS__)
#define error(format, ...)   telem_printf(LOG_LEVEL_ERROR,   LOG_GROUP_MAIN, format, ##__VA_ARGS__)

#define debug_ap(format, ...)   telem_printf(LOG_LEVEL_DEBUG,   LOG_GROUP_AP, format, ##__VA_ARGS__)
#define info_ap(format, ...)    telem_printf(LOG_LEVEL_INFO,    LOG_GROUP_AP, format, ##__VA_ARGS__)
#define warning_ap(format, ...) telem_printf(LOG_LEVEL_WARNING, LOG_GROUP_AP, format, ##__VA_ARGS__)
#define error_ap(format, ...)   telem_printf(LOG_LEVEL_ERROR,   LOG_GROUP_AP, format, ##__VA_ARGS__)

#endif