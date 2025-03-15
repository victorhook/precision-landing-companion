#ifndef AP_H
#define AP_H

#include <stdint.h>

typedef struct
{
    // General states
    uint32_t last_heartbeat;
    uint8_t mav_mode_flag;
    uint8_t mav_state;
    uint8_t flight_mode;
    
    // Attitude
    float roll;
    float pitch;
    float yaw;
    float roll_speed;
    float pitch_speed;
    float yaw_speed;
    
    // RF
    float rngfnd_dist_m;

    // Bat
    float bat_voltage;
    int8_t bat_perc;

    // Position
    float local_pos_x; // [m] X Position
    float local_pos_y; // [m] Y Position
    float local_pos_z; // [m] Z Position
    float local_pos_vx; // [m/s] X Speed
    float local_pos_vy; // [m/s] Y Speed
    float local_pos_vz; // [m/s] Z Speed

    float relative_alt;
    float alt;


    // RC
    uint16_t chan1_raw;
    uint16_t chan2_raw;
    uint16_t chan3_raw;
    uint16_t chan4_raw;
    uint16_t chan5_raw;
    uint16_t chan6_raw;
    uint16_t chan7_raw;
    uint16_t chan8_raw;
    uint16_t chan9_raw;
    uint16_t chan10_raw;
    uint8_t rssi;
}__attribute__((packed)) ap_t;


#endif