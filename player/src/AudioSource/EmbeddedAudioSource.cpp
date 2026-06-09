#include "EmbeddedAudioSource.h"
#include "../ChannelData/EmbeddedChannelData.h"

EmbeddedAudioSource::EmbeddedAudioSource(EmbeddedChannelData *channelData): mChannelData(channelData)
{
}

int EmbeddedAudioSource::getAudioSamples(uint8_t **buffer, size_t &bufferSize, int currentAudioSample)
{
  (void) currentAudioSample;
  return (int)mChannelData->getNextAudioChunk(buffer, bufferSize);
}
