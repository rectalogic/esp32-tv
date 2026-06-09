
#include <Arduino.h>
#include "SDCardVideoSource.h"
#include "../AVIParser/AVIParser.h"
#include "../ChannelData/SDCardChannelData.h"

SDCardVideoSource::SDCardVideoSource(SDCardChannelData *mChannelData) : mChannelData(mChannelData)
{
}

void SDCardVideoSource::start()
{
  // nothing to do!
}

bool SDCardVideoSource::getVideoFrame(uint8_t **buffer, size_t &bufferLength, size_t &frameLength)
{
  if (mState == VideoPlayerState::STOPPED || mState == VideoPlayerState::STATIC)
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    Serial.println("SDCardVideoSource::getVideoFrame: video is stopped or static");
    return false;
  }
  if (mState == VideoPlayerState::PAUSED)
  {
    // video time is not passing, so keep moving the start time forward so it is now
    mLastAudioTimeUpdateMs = millis();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    Serial.println("SDCardVideoSource::getVideoFrame: video is paused");
    return false;
  }
  // work out the video time from a combination of the currentAudioSample and the elapsed time
  int elapsedTime = millis() - mLastAudioTimeUpdateMs;
  int videoTime = mAudioTimeMs + elapsedTime;
  unsigned int frameDurationMs = mChannelData->getFrameDurationMs();
  if (frameDurationMs == 0)
  {
    Serial.println("SDCardVideoSource::getVideoFrame: AVI frame duration is not available");
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
    frameLength = mChannelData->getNextVideoChunk((uint8_t **)buffer, bufferLength);
    if (frameLength == 0) {
      return false;
    }
  }
  return true;
}
