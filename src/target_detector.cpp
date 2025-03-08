#include "target_detector.h"
#include <math.h>
#include "hal.h"
#include "log.h"

#define _framesRequiredForLock 5
#define _allowedMissedFrames 2

#define TARGET_TAG_ID 0


TargetDetector::TargetDetector()
: tagLock(_framesRequiredForLock, _allowedMissedFrames), targetTagId(TARGET_TAG_ID)
{
    
}

bool TargetDetector::init(const uint16_t img_width, const uint16_t img_height, const uint8_t camera_fov)
{
    this->img_width = img_width;
    this->img_height = img_height;
    this->camera_fov = camera_fov;

    // Tag detection
    tf = tag16h5_create();
    //tf = tagStandard41h12_create();

    // Create AprilTag detector object
    td = apriltag_detector_create();
    
    // Add tag family to the detector
    apriltag_detector_add_family(td, tf);

    // Tag detector configs
    td->quad_sigma = 0.6;
    td->quad_decimate = 4.0;
    td->refine_edges = 1;
    td->decode_sharpening = 0.25;
    td->nthreads = 1;
    td->debug = 0;

    return true;
}

bool TargetDetector::getLockedTag(tag_t* tag)
{
    return tagTracker.getTag(targetTagId, tag);
}

void TargetDetector::detectTagsInImage(const uint32_t width, const uint32_t height, const uint32_t stride, const uint8_t* buf)
{
    image_u8_t im = {
        .width = (int32_t) width,
        .height = (int32_t) height,
        .stride = (int32_t) stride,
        .buf = (uint8_t*) buf
    };

    // Remove old tags
    tagTracker.removeOldTags();
  
    // Detect
    zarray_t *detections = apriltag_detector_detect(td, &im);
    m_nbr_of_tags_detected = zarray_size(detections);
    bool targetTagFound = false;
    tag_t tag;

    if (m_nbr_of_tags_detected > 0)
    {
        //printf("Detections: ");
        for (int i = 0; i < m_nbr_of_tags_detected; i++) {
            apriltag_detection_t *det;
            zarray_get(detections, i, &det);
            
            // Update position
            for (int point = 0; point < 4; point++)
            {
                tag.p[point].x = det->p[point][0];
                tag.p[point].y = det->p[point][1];
            }
            tag.center.x = det->c[0];
            tag.center.y = det->c[1];

            // Update timers
            tag.id = det->id;
            tag.lastSeen = hal_millis();

            tagTracker.updateTag(&tag);

            if (isTargetTag(tag.id))
            {
                targetTagFound = true;
            }
        }
    }

    // Update tag lock hysteresis
    if (tagLock.update(targetTagFound))
    {
        getLockedTag(&tag);
        calculateLandingTarget(tag);
        info("[%d] (%f, %f) (%f, %f)\n", landing_target.id, landing_target.angle_x, landing_target.angle_y, landing_target.distance, landing_target.size_x, landing_target.size_y);
    }
    //tagTracker.printTags();

    // Free memory
    apriltag_detections_destroy(detections);
}

bool TargetDetector::isTargetTag(const uint16_t id)
{
    return id == targetTagId;
}

void TargetDetector::setTagDetectionParams(const tag_detection_params_t* params)
{
    td->quad_decimate = params->quad_decimate;
    td->quad_sigma = params->quad_sigma;
    td->refine_edges = params->refine_edges;
    td->decode_sharpening = params->decode_sharpening;
}

tag_detection_params_t TargetDetector::getTagDetectionParams()
{
    return
    {
        .quad_decimate = td->quad_decimate,
        .quad_sigma = td->quad_sigma,
        .refine_edges = td->refine_edges,
        .decode_sharpening = td->decode_sharpening,
    };
}

uint8_t TargetDetector::getTagsDetected(tag_t* tags)
{
    memcpy(tags, &tagTracker.tags, (tagTracker.tag_count*sizeof(tag_t)));
    return tagTracker.tag_count;
}

void TargetDetector::getLandingTarget(landing_target_t* target)
{
    memcpy(target, &landing_target, sizeof(landing_target_t));
}


void TargetDetector::calculateLandingTarget(const tag_t& tag)
{
    float tag_real_width = 0.1; // In meters

    // Compute the normalized offset from the center of the image:
    float tag_x = tag.center.x;
    float tag_y = tag.center.y;
    float x_offset = (tag_x - ((float) img_width / 2.0)) / ((float) img_width / 2.0);  // Normalize to [-1, 1]
    float y_offset = (tag_y - ((float) img_height / 2.0)) / ((float) img_height / 2.0);

    float angle_x = x_offset * ((float) camera_fov / 2.0);
    float angle_y = y_offset * ((float) camera_fov / 2.0);

    // Measure the Tag Size in Pixels
    float tag_pixel_width = sqrt(pow(tag.p[1].x - tag.p[0].x, 2) + pow(tag.p[1].y - tag.p[0].y, 2));
    float tag_pixel_height = sqrt(pow(tag.p[2].x - tag.p[1].x, 2) + pow(tag.p[2].y - tag.p[1].y, 2));
    // Convert to radians
    float size_x = (tag_pixel_width / img_width) * (float) camera_fov;
    float size_y = (tag_pixel_height / img_height) * (float) camera_fov;

    // Convert to Distance Using Known Tag Size
    float focal_length_px = ((float) img_width / 2.0) / tan((float) camera_fov / 2.0);  // Compute focal length in pixels
    float distance = (tag_real_width * focal_length_px) / tag_pixel_width;

    // Update target
    landing_target.angle_x = angle_x;
    landing_target.angle_y = angle_y;
    landing_target.distance = distance;
    landing_target.size_x = size_x;
    landing_target.size_y = size_y;
}

bool TargetDetector::hasLock()
{
    return tagLock.hasLock();
}

// -- Lock -- //

HysteresisLock::HysteresisLock(const uint32_t framesRequiredForLock, const uint32_t allowedMissedFrames)
: framesRequiredForLock(framesRequiredForLock), allowedMissedFrames(allowedMissedFrames),
missedConsecutiveFrames(0)
{}

HysteresisLock::~HysteresisLock()
{}

bool HysteresisLock::update(const bool tagIdAvailable)
{
    //printf("%d, %d, %d, %d\n", tagIdAvailable, missedConsecutiveFrames, okFrames, hasLock());
    if (tagIdAvailable)
    {
        missedConsecutiveFrames = 0;
        okFrames++;
    }
    else
    {
        missedConsecutiveFrames++;
        if (missedConsecutiveFrames >= allowedMissedFrames)
        {
            okFrames = 0;
        }
    }
    return hasLock();
}

bool HysteresisLock::hasLock()
{
    return okFrames >= framesRequiredForLock;
}

// -- TagTracker -- //
// ✅ Constructor: Initialize count
TagTracker::TagTracker() : tag_count(0) {}

// ✅ Check if a tag exists
bool TagTracker::tagExists(int id)
{
    for (int i = 0; i < tag_count; i++)
    {
        if (tags[i].id == id)
            return true;
    }
    return false;
}

// ✅ Remove old tags (if timeout exceeded)
void TagTracker::removeOldTags()
{
    for (int i = 0; i < tag_count;)
    {
        if ((hal_millis() - tags[i].lastSeen) > TIMEOUT_MS)
        {
            removeAt(i);
        }
        else
        {
            i++;
        }
    }
}

// ✅ Add or update a tag
void TagTracker::updateTag(tag_t* tag)
{
    // If tag exists, update timestamp and position
    for (int i = 0; i < tag_count; i++)
    {
        if (tags[i].id == tag->id)
        {
            memcpy(&tags[i], tag, sizeof(tag_t));
            tags[i].lastSeen = hal_millis();
            return;
        }
    }

    // Add new tag if space available
    if (tag_count < MAX_TAGS)
    {
        memcpy(&tags[tag_count], tag, sizeof(tag_t));
        tag_count++;
    }
}

bool TagTracker::getTag(const uint16_t id, tag_t* tag)
{
    // If tag exists, update timestamp
    for (int i = 0; i < tag_count; i++)
    {
        if (tags[i].id == tag->id)
        {
            memcpy(tag, &tags[i], sizeof(tag_t));
            return true;
        }
    }
    return false;
}

// ✅ Debug: Print tracked tags
void TagTracker::printTags()
{
    info("Tracked Tags: ");
    for (int i = 0; i < tag_count; i++)
    {
        info("[%d] ", tags[i].id);
    }
    info("\n");
}

// ✅ Remove tag at index (shift array left)
void TagTracker::removeAt(int index)
{
    for (int i = index; i < tag_count - 1; i++)
    {
        tags[i] = tags[i + 1];
    }
    tag_count--;
}
