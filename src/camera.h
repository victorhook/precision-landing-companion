#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>

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
    float quad_decimate;
    float quad_sigma;
    bool  refine_edges;
    float decode_sharpening;
}__attribute__((packed)) tag_detection_params_t;


class Camera
{
    public:
        Camera();
        bool init();
        void capture();
        int getFps();
        uint32_t getThrownFrames();
        virtual uint8_t getTagsDetected(tag_position* tags) = 0;
        virtual void setTagDetectionParams(const tag_detection_params_t* params) = 0;
        virtual tag_detection_params_t getTagDetectionParams() = 0;

    protected:
        bool m_isInitialized;
        uint32_t m_thrown_frames;
        virtual void initializeTagDetection() = 0;
        virtual bool doInit() = 0;
        virtual bool doCapture() = 0;
        virtual const char* name() = 0;
        virtual void sendImageOverUDP(const uint8_t* buf, const uint32_t len) = 0;
        virtual void detectTagsInImage(const uint32_t width, const uint32_t height, const uint32_t stride, const uint8_t* buf) = 0;
        tag_position m_tags_detected[10];
        uint8_t m_nbr_of_tags_detected;

    private:
        int m_fps_counter;
        int m_fps;
        uint32_t m_last_fps_update;
};

#endif