#include "EmbeddedVideo.h"

#ifdef USE_EMBED

extern const uint8_t video_avi_start[] asm("_binary_src_video_avi_start");
extern const uint8_t video_avi_end[] asm("_binary_src_video_avi_end");

const uint8_t *EMBEDDED_VIDEO_DATA = video_avi_start;

const size_t EMBEDDED_VIDEO_LENGTH = video_avi_end - video_avi_start;
const char *EMBEDDED_VIDEO_NAME = "video.avi";
#endif
