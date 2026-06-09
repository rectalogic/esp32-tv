#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "AVIParser.h"

struct ChunkHeader
{
  char chunkId[4];
  unsigned int chunkSize;
};

static unsigned int readUInt32LE(const uint8_t *data)
{
  return ((unsigned int)data[0]) |
         ((unsigned int)data[1] << 8) |
         ((unsigned int)data[2] << 16) |
         ((unsigned int)data[3] << 24);
}

static bool isChunkId(const char *chunkId, const char *expected)
{
  return strncmp(chunkId, expected, 4) == 0;
}

bool AVIParser::readChunk(ChunkHeader *header)
{
  uint8_t chunkSize[4];
  if (!readBytes(&header->chunkId, 4) || !readBytes(chunkSize, 4))
  {
    return false;
  }
  header->chunkSize = readUInt32LE(chunkSize);
  // Serial.printf("ChunkId %c%c%c%c, size %u\n",
  //        header->chunkId[0], header->chunkId[1],
  //        header->chunkId[2], header->chunkId[3],
  //        header->chunkSize);
  return true;
}

AVIParser::AVIParser(std::string fname, AVIChunkType requiredChunkType): mFileName(fname), mRequiredChunkType(requiredChunkType)
{
}

AVIParser::AVIParser(const uint8_t *data, size_t dataLength, AVIChunkType requiredChunkType): mRequiredChunkType(requiredChunkType), mData(data), mDataLength(dataLength)
{
}

AVIParser::~AVIParser()
{
  if (mFile)
  {
    fclose(mFile);
  }
}

bool AVIParser::readBytes(void *buffer, size_t length)
{
  if (mFile)
  {
    return fread(buffer, 1, length, mFile) == length;
  }
  if (mData)
  {
    if (mDataPosition + length > mDataLength)
    {
      return false;
    }
    memcpy(buffer, mData + mDataPosition, length);
    mDataPosition += length;
    return true;
  }
  return false;
}

bool AVIParser::seek(long offset, int whence)
{
  if (mFile)
  {
    return fseek(mFile, offset, whence) == 0;
  }
  if (!mData)
  {
    return false;
  }

  long basePosition = 0;
  switch (whence)
  {
  case SEEK_SET:
    basePosition = 0;
    break;
  case SEEK_CUR:
    basePosition = (long)mDataPosition;
    break;
  case SEEK_END:
    basePosition = (long)mDataLength;
    break;
  default:
    return false;
  }

  long nextPosition = basePosition + offset;
  if (nextPosition < 0 || (size_t)nextPosition > mDataLength)
  {
    return false;
  }
  mDataPosition = (size_t)nextPosition;
  return true;
}

long AVIParser::tell()
{
  if (mFile)
  {
    return ftell(mFile);
  }
  if (mData)
  {
    return (long)mDataPosition;
  }
  return -1;
}

bool AVIParser::isOpen()
{
  return mFile != NULL || mData != NULL;
}

bool AVIParser::isMoviListChunk(unsigned int chunkSize)
{
  char listType[4];
  if (!readBytes(&listType, 4))
  {
    return false;
  }
  chunkSize -= 4;
  Serial.printf("LIST type %c%c%c%c\n",
                listType[0], listType[1],
                listType[2], listType[3]);
  // check for the movi list - contains the video frames and audio data
  if (isChunkId(listType, "movi"))
  {
    Serial.printf("Found movi list.\n");
    Serial.printf("List Chunk Length: %d\n", chunkSize);
    mMoviListPosition = tell();
    mMoviListLength = chunkSize;
    return true;
  }
  else if (isChunkId(listType, "hdrl") || isChunkId(listType, "strl"))
  {
    return scanList(chunkSize);
  }
  else
  {
    // skip the rest of the bytes
    seek(chunkSize, SEEK_CUR);
  }
  return false;
}

bool AVIParser::scanList(unsigned int chunkSize)
{
  long listEnd = tell() + chunkSize;
  ChunkHeader header;
  while (tell() >= 0 && tell() < listEnd)
  {
    if (!readChunk(&header))
    {
      return false;
    }

    if (isChunkId(header.chunkId, "LIST"))
    {
      if (isMoviListChunk(header.chunkSize))
      {
        return true;
      }
    }
    else if (isChunkId(header.chunkId, "avih"))
    {
      parseAviHeader(header.chunkSize);
    }
    else if (isChunkId(header.chunkId, "strh"))
    {
      parseStreamHeader(header.chunkSize);
    }
    else
    {
      seek(header.chunkSize, SEEK_CUR);
    }

    if (header.chunkSize % 2 != 0)
    {
      seek(1, SEEK_CUR);
    }
  }
  return false;
}

void AVIParser::parseAviHeader(unsigned int chunkSize)
{
  uint8_t header[56];
  size_t bytesToRead = min((size_t)chunkSize, sizeof(header));
  if (!readBytes(header, bytesToRead))
  {
    return;
  }
  if (chunkSize > bytesToRead)
  {
    seek(chunkSize - bytesToRead, SEEK_CUR);
  }
  if (bytesToRead < 4)
  {
    return;
  }

  unsigned int microSecPerFrame = readUInt32LE(header);
  if (microSecPerFrame > 0)
  {
    mFrameDurationMs = max(1U, (microSecPerFrame + 999) / 1000);
    Serial.printf("AVI frame duration: %u ms\n", mFrameDurationMs);
  }
}

void AVIParser::parseStreamHeader(unsigned int chunkSize)
{
  uint8_t header[56];
  size_t bytesToRead = min((size_t)chunkSize, sizeof(header));
  if (!readBytes(header, bytesToRead))
  {
    return;
  }
  if (chunkSize > bytesToRead)
  {
    seek(chunkSize - bytesToRead, SEEK_CUR);
  }
  if (bytesToRead < 32 || !isChunkId((const char *)header, "vids"))
  {
    return;
  }

  unsigned int scale = readUInt32LE(header + 20);
  unsigned int rate = readUInt32LE(header + 24);
  if (scale > 0 && rate > 0)
  {
    mFrameDurationMs = max(1U, (unsigned int)(((unsigned long long)scale * 1000 + rate - 1) / rate));
    Serial.printf("AVI video stream frame duration: %u ms\n", mFrameDurationMs);
  }
}

bool AVIParser::open()
{
  mMoviListPosition = 0;
  mDataPosition = 0;
  mFrameDurationMs = 0;
  if (!mData)
  {
    mFile = fopen(mFileName.c_str(), "rb");
    if (!mFile)
    {
      Serial.printf("Failed to open file.\n");
      return false;
    }
  }
  // check the file is valid
  ChunkHeader header;
  // Read RIFF header
  if (!readChunk(&header))
  {
    Serial.println("Failed to read RIFF header.");
    return false;
  }
  if (strncmp(header.chunkId, "RIFF", 4) != 0)
  {
    Serial.println("Not a valid AVI file.");
    if (mFile)
    {
      fclose(mFile);
      mFile = NULL;
    }
    return false;
  }
  else
  {
    Serial.printf("RIFF header found.\n");
  }
  // next four bytes are the RIFF type which should be 'AVI '
  char riffType[4];
  if (!readBytes(&riffType, 4))
  {
    Serial.println("Failed to read RIFF type.");
    return false;
  }
  if (strncmp(riffType, "AVI ", 4) != 0)
  {
    Serial.println("Not a valid AVI file.");
    if (mFile)
    {
      fclose(mFile);
      mFile = NULL;
    }
    return false;
  }
  else
  {
    Serial.println("RIFF Type is AVI.");
  }

  // now read each chunk and find the movi list
  scanList(header.chunkSize - 4);
  // did we find the list?
  if (mMoviListPosition == 0)
  {
    Serial.printf("Failed to find the movi list.\n");
    if (mFile)
    {
      fclose(mFile);
      mFile = NULL;
    }
    return false;
  }
  // keep the file open for reading the frames
  return true;
}

unsigned int AVIParser::getFrameDurationMs()
{
  return mFrameDurationMs;
}

size_t AVIParser::getNextChunk(uint8_t **buffer, size_t &bufferLength)
{
  // check if the file is open
  if (!isOpen())
  {
    Serial.println("No file open.");
    return 0;
  }
  // did we find the movi list?
  if (mMoviListPosition == 0) {
    Serial.println("No movi list found.");
    return 0;
  }
  // get the next chunk of data from the list
  ChunkHeader header;
  while (mMoviListLength > 0)
  {
    if (!readChunk(&header))
    {
      return 0;
    }
    mMoviListLength -= 8;
    bool isVideoChunk = strncmp(header.chunkId, "00dc", 4) == 0;
    bool isAudioChunk = strncmp(header.chunkId, "01wb", 4) == 0;
    if (mRequiredChunkType == AVIChunkType::VIDEO && isVideoChunk ||
        mRequiredChunkType == AVIChunkType::AUDIO && isAudioChunk)
    {
      // we've got the required chunk - copy it into the provided buffer
      // reallocate the buffer if necessary
      if (header.chunkSize > bufferLength)
      {
        *buffer = (uint8_t *)realloc(*buffer, header.chunkSize);
        bufferLength = header.chunkSize;
      }
      // copy the chunk data
      if (!readBytes(*buffer, header.chunkSize))
      {
        return 0;
      }
      mMoviListLength -= header.chunkSize;
      // handle any padding bytes
      if (header.chunkSize % 2 != 0)
      {
        seek(1, SEEK_CUR);
        mMoviListLength--;
      }
      return header.chunkSize;
    }
    else
    {
      // the data is not what was required - skip over the chunk
      seek(header.chunkSize, SEEK_CUR);
      mMoviListLength -= header.chunkSize;
    }
    // handle any padding bytes
    if (header.chunkSize % 2 != 0)
    {
      seek(1, SEEK_CUR);
      mMoviListLength--;
    }
  }
  // no more chunks
  Serial.println("No more data");
  return 0;
}
