#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "hal.h"

#ifdef LINUX
    #include "linux/linux_camera.h"
#else
    #include "esp32/esp32_camera.h"
#endif



#endif