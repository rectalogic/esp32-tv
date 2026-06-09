#pragma once

#include <Arduino.h>

#ifdef USE_EMBED

#ifndef EMBEDDED_VIDEO
#error "USE_EMBED requires EMBEDDED_VIDEO to name the AVI resource"
#endif

extern const uint8_t EMBEDDED_VIDEO_DATA[];
extern const size_t EMBEDDED_VIDEO_LENGTH;
extern const char *EMBEDDED_VIDEO_NAME;

#endif
