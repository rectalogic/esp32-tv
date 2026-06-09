#pragma once

#include "AudioSource.h"

class EmbeddedChannelData;

class EmbeddedAudioSource : public AudioSource
{
private:
  EmbeddedChannelData *mChannelData = NULL;

public:
  EmbeddedAudioSource(EmbeddedChannelData *channelData);
  int getAudioSamples(uint8_t **buffer, size_t &bufferSize, int currentAudioSample);
};
