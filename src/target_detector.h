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
    uint16_t id;
    uint32_t lastSeen;

    // The center of the detection in image pixel coordinates.
    point_2f center;

    // The corners of the tag in image pixel coordinates. These always
    // wrap counter-clock wise around the tag.
    point_2f p[4];
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


#define MAX_TAGS 10          // Maximum number of tracked tags
#define TIMEOUT_MS 500      // Remove tag if unseen for 5 seconds

class TagTracker
{
public:
    TagTracker();  // Constructor
    void updateTag(tag_t* tag);    // ✅ Add or update a tag
    void removeOldTags();      // ✅ Remove tags that timed out
    void printTags();          // ✅ Debug: Print tracked tags
    bool getTag(const uint16_t id, tag_t* tag);

    friend class TargetDetector;
private:
    tag_t tags[MAX_TAGS];  // Array of tracked tags
    int tag_count;       // Number of tracked tags

    void removeAt(int index);  // ✅ Remove tag at index
    bool tagExists(int id);    // ✅ Check if tag exists
};


class HysteresisLock
{
    public:
        HysteresisLock(const uint32_t framesRequiredForLock, const uint32_t allowedMissedFrames);
        ~HysteresisLock();
        /** Returns true if a lock is found */
        bool update(const bool tagIdAvailable = false);
        bool hasLock();

    private:
        const uint32_t framesRequiredForLock;
        const uint32_t allowedMissedFrames;
        uint32_t missedConsecutiveFrames;
        uint32_t okFrames;

};

class TargetDetector
{
    public:
        TargetDetector();
        bool init(const uint16_t img_width, const uint16_t img_height, const uint8_t camera_fov);
        void detectTagsInImage(const uint32_t width, const uint32_t height, const uint32_t stride, const uint8_t* buf);
        uint8_t getTagsDetected(tag_t* tags);
        void setTagDetectionParams(const tag_detection_params_t* params);
        tag_detection_params_t getTagDetectionParams();
        landing_target_t getLandingTarget();

        bool hasLock();
        bool getLockedTag(tag_t* tag);

    private:
        apriltag_family_t* tf;
        apriltag_detector_t* td;
        uint8_t m_nbr_of_tags_detected;
        landing_target_t landing_target;
        uint16_t targetTagId;

        HysteresisLock tagLock;
        TagTracker tagTracker;

        uint16_t img_width;
        uint16_t img_height;
        uint8_t camera_fov;

        landing_target_t calculateLandingTarget(const tag_t& tag);
        bool isTargetTag(const uint16_t id);
};


#endif
