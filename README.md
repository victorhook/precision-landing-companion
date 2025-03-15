# Precision Landing Companion

This project aims to create a high-precision landing companion for Ardupilot. The hardware is based on an ESP32-S3 with a camera that communicate to AP with MAVLink over UART.

## TODO
- [ ] Image "coordinate"
- [X] MAVLink message
- [ ] General procedure, how to land, switch etc
- [X] Tag filtering, "lock" 
- [X] WiFi hotspot
- [X] ESP32 serial port
- [ ] Settings that persists over boot (in flash)


## MAVLink parameters
```
PLND_ENABLED     1     # Precision Land Enabled
PLND_TYPE        1     # MAVLink
PLND_YAW_ALIGN   0     # Sensor yaw alignment
PLND_LAND_OFS_X  0     # Land offset forward
PLND_LAND_OFS_Y  0     # Land offset right
PLND_EST_TYPE    0     # 0 = RawSensor, 1 = KalmanFilter
PLND_ACC_P_NSE   0.5   # Kalman Filter Accelerometer Noise
PLND_CAM_POS_X   0     # Camera X position offset, Positive X is forward of the origin.
PLND_CAM_POS_Y   0     # Camera Y position offset, Positive Y is to the right of the origin.
PLND_CAM_POS_Z   0     # Camera Z position offset, Positive Z is down from the origin.
PLND_LAG         0.05  # Precision Landing sensor lag (seconds)
PLND_XY_DIST_MAX 0     # The vehicle will not start descending if the landing target is detected and it is further than this many meters away. Set 0 to always descend.
PLND_STRICT      1     # How strictly should the vehicle land on the target if target is lost. 0 = Land Vertically (Not strict), 1 = Retry Landing(Normal Strictness), 2 = Do not land (just Hover) (Very Strict)
PLND_RET_MAX     3     # PrecLand Maximum number of retires for a failed landing
PLND_TIMEOUT     0.5   # PrecLand retry timeout
PLND_RET_BEHAVE  0     # PrecLand retry behaviour. 0 = Go to the last location where landing target was detected. 1 = Go towards the approximate location of the detected landing target
PLND_ALT_MIN     0.015 # PrecLand minimum alt for retry. Vehicle will continue landing vertically even if target is lost below this height. This needs a rangefinder to work. Set to zero to disable this.
PLND_ALT_MAX     0.75  # PrecLand maximum alt for retry
PLND_OPTIONS     0    # Precision Landing Extra Options. 0 = Moving Landing Target. 1 = Allow Precision Landing after manual reposition. 2 = Maintain high speed in final descent
```