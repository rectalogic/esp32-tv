#pragma once

#include "ChannelData.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class AVIParser;

class EmbeddedChannelData : public ChannelData
{
private:
  const uint8_t *mAviData = NULL;
  size_t mAviDataLength = 0;
  const char *mAviName = NULL;
  AVIParser *mCurrentChannelAudioParser = NULL;
  AVIParser *mCurrentChannelVideoParser = NULL;
  SemaphoreHandle_t mParserMutex = NULL;

public:
  EmbeddedChannelData(const uint8_t *aviData, size_t aviDataLength, const char *aviName);
  ~EmbeddedChannelData();
  bool fetchChannelData();
  int getChannelCount() {
    return 1;
  };
  size_t getNextAudioChunk(uint8_t **buffer, size_t &bufferLength);
  size_t getNextVideoChunk(uint8_t **buffer, size_t &bufferLength);
  unsigned int getFrameDurationMs();
  void setChannel(int channel);
};
