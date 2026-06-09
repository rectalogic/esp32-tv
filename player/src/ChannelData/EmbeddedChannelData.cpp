#include <Arduino.h>
#include "EmbeddedChannelData.h"
#include "../AVIParser/AVIParser.h"

EmbeddedChannelData::EmbeddedChannelData(const uint8_t *aviData, size_t aviDataLength, const char *aviName): mAviData(aviData), mAviDataLength(aviDataLength), mAviName(aviName)
{
  mParserMutex = xSemaphoreCreateMutex();
}

EmbeddedChannelData::~EmbeddedChannelData()
{
  if (mParserMutex != NULL)
  {
    xSemaphoreTake(mParserMutex, portMAX_DELAY);
  }
  if (mCurrentChannelAudioParser)
  {
    delete mCurrentChannelAudioParser;
    mCurrentChannelAudioParser = NULL;
  }
  if (mCurrentChannelVideoParser)
  {
    delete mCurrentChannelVideoParser;
    mCurrentChannelVideoParser = NULL;
  }
  if (mParserMutex != NULL)
  {
    xSemaphoreGive(mParserMutex);
    vSemaphoreDelete(mParserMutex);
    mParserMutex = NULL;
  }
}

bool EmbeddedChannelData::fetchChannelData()
{
  if (mAviData == NULL || mAviDataLength == 0)
  {
    Serial.println("No embedded AVI data");
    return false;
  }
  Serial.printf("Using embedded AVI %s (%u bytes)\n", mAviName ? mAviName : "<unnamed>", (unsigned int)mAviDataLength);
  return true;
}

void EmbeddedChannelData::setChannel(int channel)
{
  if (channel != 0)
  {
    Serial.printf("Invalid embedded channel %d\n", channel);
    return;
  }

  AVIParser *nextAudioParser = new AVIParser(mAviData, mAviDataLength, AVIChunkType::AUDIO);
  if (!nextAudioParser->open())
  {
    Serial.printf("Failed to open embedded AVI %s for audio\n", mAviName);
    delete nextAudioParser;
    return;
  }

  AVIParser *nextVideoParser = new AVIParser(mAviData, mAviDataLength, AVIChunkType::VIDEO);
  if (!nextVideoParser->open())
  {
    Serial.printf("Failed to open embedded AVI %s for video\n", mAviName);
    delete nextVideoParser;
    delete nextAudioParser;
    return;
  }

  if (mParserMutex == NULL || xSemaphoreTake(mParserMutex, portMAX_DELAY) != pdTRUE)
  {
    delete nextVideoParser;
    delete nextAudioParser;
    return;
  }

  AVIParser *oldAudioParser = mCurrentChannelAudioParser;
  AVIParser *oldVideoParser = mCurrentChannelVideoParser;
  mCurrentChannelAudioParser = nextAudioParser;
  mCurrentChannelVideoParser = nextVideoParser;
  mChannelNumber = channel;
  xSemaphoreGive(mParserMutex);

  if (oldAudioParser)
  {
    delete oldAudioParser;
  }
  if (oldVideoParser)
  {
    delete oldVideoParser;
  }
}

size_t EmbeddedChannelData::getNextAudioChunk(uint8_t **buffer, size_t &bufferLength)
{
  if (mParserMutex == NULL || xSemaphoreTake(mParserMutex, portMAX_DELAY) != pdTRUE)
  {
    return 0;
  }
  size_t chunkLength = 0;
  if (mCurrentChannelAudioParser != NULL)
  {
    chunkLength = mCurrentChannelAudioParser->getNextChunk(buffer, bufferLength);
  }
  xSemaphoreGive(mParserMutex);
  return chunkLength;
}

size_t EmbeddedChannelData::getNextVideoChunk(uint8_t **buffer, size_t &bufferLength)
{
  if (mParserMutex == NULL || xSemaphoreTake(mParserMutex, portMAX_DELAY) != pdTRUE)
  {
    return 0;
  }
  size_t chunkLength = 0;
  if (mCurrentChannelVideoParser != NULL)
  {
    chunkLength = mCurrentChannelVideoParser->getNextChunk(buffer, bufferLength);
  }
  xSemaphoreGive(mParserMutex);
  return chunkLength;
}

unsigned int EmbeddedChannelData::getFrameDurationMs()
{
  if (mParserMutex == NULL || xSemaphoreTake(mParserMutex, portMAX_DELAY) != pdTRUE)
  {
    return 0;
  }
  unsigned int frameDurationMs = 0;
  if (mCurrentChannelVideoParser != NULL)
  {
    frameDurationMs = mCurrentChannelVideoParser->getFrameDurationMs();
  }
  xSemaphoreGive(mParserMutex);
  return frameDurationMs;
}
