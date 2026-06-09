#pragma once

#include <cstddef>
#include <cstdint>
#include <stdio.h>
#include <string>

enum class AVIChunkType
{
  VIDEO, AUDIO
};

struct ChunkHeader;

class AVIParser
{
private:
  std::string mFileName;
  AVIChunkType mRequiredChunkType;
  FILE *mFile = NULL;
  const uint8_t *mData = NULL;
  size_t mDataLength = 0;
  size_t mDataPosition = 0;
  long mMoviListPosition = 0;
  long mMoviListLength = 0;
  unsigned int mFrameDurationMs = 0;

  bool isMoviListChunk(unsigned int chunkSize);
  bool scanList(unsigned int chunkSize);
  void parseAviHeader(unsigned int chunkSize);
  void parseStreamHeader(unsigned int chunkSize);
  bool readChunk(ChunkHeader *header);
  bool readBytes(void *buffer, size_t length);
  bool seek(long offset, int whence);
  long tell();
  bool isOpen();

public:
  AVIParser(std::string fname, AVIChunkType requiredChunkType);
  AVIParser(const uint8_t *data, size_t dataLength, AVIChunkType requiredChunkType);
  ~AVIParser();
  bool open();
  unsigned int getFrameDurationMs();
  size_t getNextChunk(uint8_t **buffer, size_t &bufferLength);
};
