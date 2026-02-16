#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "SDCardAudioSource.h"
#include "../AVIParser/AVIParser.h"
#include "../ChannelData/SDCardChannelData.h"

SDCardAudioSource::SDCardAudioSource(SDCardChannelData *channelData): mChannelData(channelData)
{
}

int SDCardAudioSource::getAudioSamples(uint8_t **buffer, size_t &bufferSize, int currentAudioSample)
{
  (void) currentAudioSample;
  return (int) mChannelData->getNextAudioChunk((uint8_t **) buffer, bufferSize);
}
