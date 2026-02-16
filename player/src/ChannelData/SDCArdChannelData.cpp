#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "../SDCard.h"
#include "SDCardChannelData.h"
#include "../AVIParser/AVIParser.h"

SDCardChannelData::SDCardChannelData(SDCard *sdCard, const char *aviPath): mSDCard(sdCard), mAviPath(aviPath) {
  mParserMutex = xSemaphoreCreateMutex();
}

SDCardChannelData::~SDCardChannelData() {
  if (mParserMutex != NULL) {
    xSemaphoreTake(mParserMutex, portMAX_DELAY);
  }
  if (mCurrentChannelAudioParser) {
    delete mCurrentChannelAudioParser;
    mCurrentChannelAudioParser = NULL;
  }
  if (mCurrentChannelVideoParser) {
    delete mCurrentChannelVideoParser;
    mCurrentChannelVideoParser = NULL;
  }
  if (mParserMutex != NULL) {
    xSemaphoreGive(mParserMutex);
    vSemaphoreDelete(mParserMutex);
    mParserMutex = NULL;
  }
}

bool SDCardChannelData::fetchChannelData() {
  // check the the sd card is mounted
  if (!mSDCard->isMounted()) {
    Serial.println("SD card is not mounted");
    return false;
  }
  // get the list of AVI files
  mAviFiles = mSDCard->listFiles(mAviPath, ".avi");
  if (mAviFiles.size() == 0) {
    Serial.println("No AVI files found");
    return false;
  }
  return true;
}


void SDCardChannelData::setChannel(int channel) {
  if (!mSDCard->isMounted()) {
    Serial.println("SD card is not mounted");
    return;
  }
  // check that the channel is valid
  if (channel < 0 || channel >= mAviFiles.size()) {
    Serial.printf("Invalid channel %d\n", channel);
    return;
  }
  // open the AVI file
  std::string aviFilename = mAviFiles[channel];
  Serial.printf("Opening AVI file %s\n", aviFilename.c_str());
  AVIParser *nextAudioParser = new AVIParser(aviFilename, AVIChunkType::AUDIO);
  if (!nextAudioParser->open()) {
    Serial.printf("Failed to open AVI file %s\n", aviFilename.c_str());
    delete nextAudioParser;
    nextAudioParser = NULL;
    return;
  }
  AVIParser *nextVideoParser = new AVIParser(aviFilename, AVIChunkType::VIDEO);
  if (!nextVideoParser->open()) {
    Serial.printf("Failed to open AVI file %s\n", aviFilename.c_str());
    delete nextVideoParser;
    nextVideoParser = NULL;
    delete nextAudioParser;
    nextAudioParser = NULL;
    return;
  }

  if (mParserMutex == NULL || xSemaphoreTake(mParserMutex, portMAX_DELAY) != pdTRUE) {
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

  if (oldAudioParser) {
    delete oldAudioParser;
  }
  if (oldVideoParser) {
    delete oldVideoParser;
  }
}

size_t SDCardChannelData::getNextAudioChunk(uint8_t **buffer, size_t &bufferLength) {
  if (mParserMutex == NULL || xSemaphoreTake(mParserMutex, portMAX_DELAY) != pdTRUE) {
    return 0;
  }
  size_t chunkLength = 0;
  if (mCurrentChannelAudioParser != NULL) {
    chunkLength = mCurrentChannelAudioParser->getNextChunk((uint8_t **) buffer, bufferLength);
  }
  xSemaphoreGive(mParserMutex);
  return chunkLength;
}

size_t SDCardChannelData::getNextVideoChunk(uint8_t **buffer, size_t &bufferLength) {
  if (mParserMutex == NULL || xSemaphoreTake(mParserMutex, portMAX_DELAY) != pdTRUE) {
    return 0;
  }
  size_t chunkLength = 0;
  if (mCurrentChannelVideoParser != NULL) {
    chunkLength = mCurrentChannelVideoParser->getNextChunk((uint8_t **) buffer, bufferLength);
  }
  xSemaphoreGive(mParserMutex);
  return chunkLength;
}
