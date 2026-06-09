#include "EmbeddedVideo.h"

#ifdef USE_EMBED

#define STRINGIFY_VALUE(value) #value
#define STRINGIFY(value) STRINGIFY_VALUE(value)
#define EMBEDDED_VIDEO_RESOURCE STRINGIFY(EMBEDDED_VIDEO)

#if !defined(__has_embed)
#error "USE_EMBED requires compiler support for the C++ #embed directive"
#else
#if !__has_embed(EMBEDDED_VIDEO_RESOURCE)
#error "EMBEDDED_VIDEO resource was not found"
#else
const uint8_t EMBEDDED_VIDEO_DATA[] = {
#embed EMBEDDED_VIDEO_RESOURCE
};

const size_t EMBEDDED_VIDEO_LENGTH = sizeof(EMBEDDED_VIDEO_DATA);
const char *EMBEDDED_VIDEO_NAME = STRINGIFY(EMBEDDED_VIDEO);
#endif
#endif

#endif
