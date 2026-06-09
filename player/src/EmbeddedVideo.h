#pragma once

#include <Arduino.h>

#ifdef USE_EMBED

extern const uint8_t *EMBEDDED_VIDEO_DATA;
extern const size_t EMBEDDED_VIDEO_LENGTH;
extern const char *EMBEDDED_VIDEO_NAME;

#endif
