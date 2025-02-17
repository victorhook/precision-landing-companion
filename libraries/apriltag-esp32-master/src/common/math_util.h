/* Copyright (C) 2013-2016, The Regents of The University of Michigan.
All rights reserved.
This software was developed in the APRIL Robotics Lab under the
direction of Edwin Olson, ebolson@umich.edu. This software may be
available under alternative licensing terms; contact the address above.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the Regents of The University of Michigan.
*/

#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#ifndef IRAM_ATTR
#include "esp_attr.h"
#endif
#else
#define IRAM_ATTR
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define to_radians(x) ( (x) * (M_PI / 180.0 ))
#define to_degrees(x) ( (x) * (180.0 / M_PI ))

  /* DEPRECATE, threshold meaningless without context.
static inline int dequals(float a, float b)
{
    float thresh = 1e-9;
    return (fabs(a-b) < thresh);
}
  */

static inline int IRAM_ATTR dequals_mag(float a, float b, float thresh)
{
    return (fabs(a-b) < thresh);
}

static inline int IRAM_ATTR isq(int v)
{
    return v*v;
}

static inline float IRAM_ATTR fsq(float v)
{
    return v*v;
}

static inline float IRAM_ATTR sq(float v)
{
    return v*v;
}

static inline float IRAM_ATTR sgn(float v)
{
    return (v>=0) ? 1 : -1;
}

// random number between [0, 1)
static inline float IRAM_ATTR randf()
{
    return (float)(rand() / (RAND_MAX + 1.0));
}


static inline float IRAM_ATTR signed_randf()
{
    return randf()*2 - 1;
}

// return a random integer between [0, bound)
static inline int IRAM_ATTR irand(int bound)
{
    int v = (int) (randf()*bound);
    if (v == bound)
        return (bound-1);
    //assert(v >= 0);
    //assert(v < bound);
    return v;
}

/** Map vin to [0, 2*PI) **/
static inline float IRAM_ATTR mod2pi_positive(float vin)
{
    return vin - M_2_PI * floor(vin / M_2_PI);
}

/** Map vin to [-PI, PI) **/
static inline float IRAM_ATTR mod2pi(float vin)
{
    return mod2pi_positive(vin + M_PI) - M_PI;
}

/** Return vin such that it is within PI degrees of ref **/
static inline float IRAM_ATTR mod2pi_ref(float ref, float vin)
{
    return ref + mod2pi(vin - ref);
}

/** Map vin to [0, 360) **/
static inline float IRAM_ATTR mod360_positive(float vin)
{
    return vin - 360 * floor(vin / 360);
}

/** Map vin to [-180, 180) **/
static inline float IRAM_ATTR mod360(float vin)
{
    return mod360_positive(vin + 180) - 180;
}

static inline int IRAM_ATTR mod_positive(int vin, int mod) {
    return (vin % mod + mod) % mod;
}

static inline int IRAM_ATTR theta_to_int(float theta, int max)
{
    theta = mod2pi_ref(M_PI, theta);
    int v = (int) (theta / M_2_PI * max);

    if (v == max)
        v = 0;

    assert (v >= 0 && v < max);

    return v;
}

static inline int IRAM_ATTR imin(int a, int b)
{
    return (a < b) ? a : b;
}

static inline int IRAM_ATTR imax(int a, int b)
{
    return (a > b) ? a : b;
}

static inline int64_t IRAM_ATTR imin64(int64_t a, int64_t b)
{
    return (a < b) ? a : b;
}

static inline int64_t IRAM_ATTR imax64(int64_t a, int64_t b)
{
    return (a > b) ? a : b;
}

static inline int IRAM_ATTR iclamp(int v, int minv, int maxv)
{
    return imax(minv, imin(v, maxv));
}

static inline float IRAM_ATTR dclamp(float a, float min, float max)
{
    if (a < min)
        return min;
    if (a > max)
        return max;
    return a;
}

static inline int IRAM_ATTR fltcmp (float f1, float f2)
{
    float epsilon = f1-f2;
    if (epsilon < 0.0)
        return -1;
    else if (epsilon > 0.0)
        return  1;
    else
        return  0;
}

static inline int IRAM_ATTR dblcmp (float d1, float d2)
{
    float epsilon = d1-d2;
    if (epsilon < 0.0)
        return -1;
    else if (epsilon > 0.0)
        return  1;
    else
        return  0;
}

#ifdef __cplusplus
}
#endif
