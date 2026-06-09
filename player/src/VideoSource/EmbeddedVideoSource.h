#pragma once

#include "VideoSource.h"

class EmbeddedChannelData;

class EmbeddedVideoSource : public VideoSource
{
private:
  EmbeddedChannelData *mChannelData;
  int mFrameCount = 0;

public:
  EmbeddedVideoSource(EmbeddedChannelData *channelData);
  void start();
  bool getVideoFrame(uint8_t **buffer, size_t &bufferLength, size_t &frameLength);
  void setChannel(int channel) {
    (void) channel;
    mFrameCount = 0;
  }
};
