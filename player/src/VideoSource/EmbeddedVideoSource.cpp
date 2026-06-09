#include <Arduino.h>
#include "EmbeddedVideoSource.h"
#include "../ChannelData/EmbeddedChannelData.h"

EmbeddedVideoSource::EmbeddedVideoSource(EmbeddedChannelData *channelData): mChannelData(channelData)
{
}

void EmbeddedVideoSource::start()
{
  // nothing to do
}

bool EmbeddedVideoSource::getVideoFrame(uint8_t **buffer, size_t &bufferLength, size_t &frameLength)
{
  if (mState == VideoPlayerState::STOPPED || mState == VideoPlayerState::STATIC)
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    return false;
  }
  if (mState == VideoPlayerState::PAUSED)
  {
    mLastAudioTimeUpdateMs = millis();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    return false;
  }

  int elapsedTime = millis() - mLastAudioTimeUpdateMs;
  int videoTime = mAudioTimeMs + elapsedTime;
  unsigned int frameDurationMs = mChannelData->getFrameDurationMs();
  if (frameDurationMs == 0)
  {
    Serial.println("EmbeddedVideoSource::getVideoFrame: AVI frame duration is not available");
    vTaskDelay(100 / portTICK_PERIOD_MS);
    return false;
  }
  int frameTime = mFrameCount * frameDurationMs;
  if (videoTime <= frameTime)
  {
    return false;
  }
  while (videoTime > mFrameCount * frameDurationMs)
  {
    mFrameCount++;
    frameLength = mChannelData->getNextVideoChunk(buffer, bufferLength);
    if (frameLength == 0)
    {
      return false;
    }
  }
  return true;
}
