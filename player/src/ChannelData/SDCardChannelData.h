#pragma once

#include "ChannelData.h"
#include <vector>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class SDCard;
class AVIParser;

class SDCardChannelData : public ChannelData
{
private:
  const char *mChannelInfoURL = NULL;
  std::vector<std::string> mAviFiles;
  AVIParser *mCurrentChannelAudioParser = NULL;
  AVIParser *mCurrentChannelVideoParser = NULL;
  SemaphoreHandle_t mParserMutex = NULL;
  SDCard *mSDCard;
  const char *mAviPath;
public:
  SDCardChannelData(SDCard *sdCard, const char *aviPath);
  ~SDCardChannelData();
  bool fetchChannelData();
  int getChannelCount() {
    return mAviFiles.size();
  };
  int getChannelLength(int channelIndex) {
    // we don't know the length of the AVI file
    return -1;
  };
  AVIParser *getAudioParser() {
    return mCurrentChannelAudioParser;
  };
  AVIParser *getVideoParser() {
    return mCurrentChannelVideoParser;
  };
  size_t getNextAudioChunk(uint8_t **buffer, size_t &bufferLength);
  size_t getNextVideoChunk(uint8_t **buffer, size_t &bufferLength);
  unsigned int getFrameDurationMs();
  void setChannel(int channel);
};
