#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <string.h>
#include <stddef.h>


#ifdef LINUX
    #include "linux/hal_linux.h"
#else
    #include "esp32/hal_esp32.h"
#endif




#endif