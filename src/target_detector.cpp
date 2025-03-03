#include "target_detector.h"
#include <math.h>


TargetDetector::TargetDetector()
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


void TargetDetector::detectTagsInImage(const uint32_t width, const uint32_t height, const uint32_t stride, const uint8_t* buf)
{
    image_u8_t im = {
        .width = (int32_t) width,
        .height = (int32_t) height,
        .stride = (int32_t) stride,
        .buf = (uint8_t*) buf
    };
  
    // Detect
    zarray_t *detections = apriltag_detector_detect(td, &im);

    m_nbr_of_tags_detected = zarray_size(detections);
    if (m_nbr_of_tags_detected > 0)
    {
        //printf("Detections: ");
        for (int i = 0; i < m_nbr_of_tags_detected; i++) {
            apriltag_detection_t *det;
            zarray_get(detections, i, &det);
            
            tag_position* tag = &m_tags_detected[i];

            for (int point = 0; point < 4; point++)
            {
                tag->p[point].x = det->p[point][0];
                tag->p[point].y = det->p[point][1];
            }
            tag->center.x = det->c[0];
            tag->center.y = det->c[1];
            //printf("%d ", det->id);
        }
        //printf("\n");
    }
    else
    {
        //printf("No tag detected\n");
    }

    // Free memory
    apriltag_detections_destroy(detections);
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

uint8_t TargetDetector::getTagsDetected(tag_position* tags)
{
    memcpy(tags, &m_tags_detected, (m_nbr_of_tags_detected*sizeof(tag_position)));
    return m_nbr_of_tags_detected;
}


landing_target_t TargetDetector::getLandingTarget()
{
    return landing_target;
}


landing_target_t TargetDetector::calculateLandingTarget(const tag_position position)
{
    landing_target_t target;
    float tag_real_width = 0.1; // In meters

    // Compute the normalized offset from the center of the image:
    float tag_x = position.center.x;
    float tag_y = position.center.y;
    float x_offset = (tag_x - ((float) img_width / 2.0)) / ((float) img_width / 2.0);  // Normalize to [-1, 1]
    float y_offset = (tag_y - ((float) img_height / 2.0)) / ((float) img_height / 2.0);

    float angle_x = x_offset * ((float) camera_fov / 2.0);
    float angle_y = y_offset * ((float) camera_fov / 2.0);

    // Measure the Tag Size in Pixels
    float tag_pixel_width = sqrt(pow(position.p[1].x - position.p[0].x, 2) + pow(position.p[1].y - position.p[0].y, 2));
    float tag_pixel_height = sqrt(pow(position.p[2].x - position.p[1].x, 2) + pow(position.p[2].y - position.p[1].y, 2));
    // Convert to radians
    float size_x = (tag_pixel_width / img_width) * (float) camera_fov;
    float size_y = (tag_pixel_height / img_height) * (float) camera_fov;

    // Convert to Distance Using Known Tag Size
    float focal_length_px = ((float) img_width / 2.0) / tan((float) camera_fov / 2.0);  // Compute focal length in pixels
    float distance = (tag_real_width * focal_length_px) / tag_pixel_width;

    // Update target
    target.angle_x = angle_x;
    target.angle_y = angle_y;
    target.distance = distance;
    target.size_x = size_x;
    target.size_y = size_y;

    return target;
}