#ifndef TARGET_DETECTOR_H
#define TARGET_DETECTOR_H

#include <stdint.h>

#include "apriltag.h"
#include "tagStandard41h12.h"
#include "tag16h5.h"

typedef struct
{
    float quad_decimate;
    float quad_sigma;
    bool  refine_edges;
    float decode_sharpening;
}__attribute__((packed)) tag_detection_params_t;


typedef struct
{
    float x;
    float y;
} point_2f;

typedef struct
{
    // The center of the detection in image pixel coordinates.
    point_2f center;

    // The corners of the tag in image pixel coordinates. These always
    // wrap counter-clock wise around the tag.
    point_2f p[4];
}__attribute__((packed)) tag_position;


typedef struct
{
    uint16_t id;
    uint32_t firstSeen;
    uint32_t lastSeen;
    uint32_t consecutiveFrames;
    tag_position current_position;
}__attribute__((packed)) tag_t;


// Struct is taken from the MAVLink documentation at https://mavlink.io/en/services/landing_target.html#camera_image_relative
typedef struct
{
    float angle_x;	// rad	-  X-axis angular offset of the target from the center of the image
    float angle_y;	// rad	-  Y-axis angular offset of the target from the center of the image
    float distance;	// m	-  Distance to the target from the vehicle
    float size_x;	// rad	-  Size of target along x-axis
    float size_y;	// rad	-  Size of target along y-axis
    uint16_t id;
} landing_target_t;


class TargetDetector
{
    public:
        TargetDetector();
        bool init(const uint16_t img_width, const uint16_t img_height, const uint8_t camera_fov);
        void detectTagsInImage(const uint32_t width, const uint32_t height, const uint32_t stride, const uint8_t* buf);
        uint8_t getTagsDetected(tag_position* tags);
        void setTagDetectionParams(const tag_detection_params_t* params);
        tag_detection_params_t getTagDetectionParams();
        landing_target_t getLandingTarget();

    private:
        apriltag_family_t* tf;
        apriltag_detector_t* td;
        tag_position m_tags_detected[10];
        uint8_t m_nbr_of_tags_detected;
        landing_target_t landing_target;

        uint16_t img_width;
        uint16_t img_height;
        uint8_t camera_fov;

        landing_target_t calculateLandingTarget(const tag_position position);
};


#endif
