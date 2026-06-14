#include <Arduino.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <streambuf>
#include <istream>
#include "AVIParser.h"

#ifdef USE_EMBED
#include "video_avi.embed"

class embedbuf : public std::streambuf {
public:
    embedbuf() {
        char* p = reinterpret_cast<char*>(const_cast<unsigned char*>(video_avi));
        setg(p, p, p + video_avi_len);
    }
};

class embedstream : public std::istream {
    embedbuf buf_;

public:
    embedstream()
        : std::istream(&buf_)
        , buf_()
    {}
};
#endif

typedef struct
{
  char chunkId[4];
  unsigned int chunkSize;
} ChunkHeader;

void readChunk(std::istream &stream, ChunkHeader *header)
{
  stream.read(header->chunkId, 4);
  stream.read(reinterpret_cast<char *>(&header->chunkSize), 4);
  // Serial.printf("ChunkId %c%c%c%c, size %u\n",
  //        header->chunkId[0], header->chunkId[1],
  //        header->chunkId[2], header->chunkId[3],
  //        header->chunkSize);
}

AVIParser::AVIParser(std::string fname, AVIChunkType requiredChunkType): mFileName(fname), mRequiredChunkType(requiredChunkType)
{
}

AVIParser::~AVIParser()
{
  delete mStream;
  mStream = nullptr;
}

bool AVIParser::isMoviListChunk(unsigned int chunkSize)
{
  char listType[4];
  mStream->read(listType, 4);
  chunkSize -= 4;
  Serial.printf("LIST type %c%c%c%c\n",
                listType[0], listType[1],
                listType[2], listType[3]);
  // check for the movi list - contains the video frames and audio data
  if (strncmp(listType, "movi", 4) == 0)
  {
    Serial.printf("Found movi list.\n");
    Serial.printf("List Chunk Length: %d\n", chunkSize);
    mMoviListPosition = mStream->tellg();
    mMoviListLength = chunkSize;
    return true;
  }
  else
  {
    // skip the rest of the bytes
    mStream->seekg(chunkSize, std::ios::cur);
  }
  return false;
}

bool AVIParser::open()
{
  #ifdef USE_EMBED
  if (mFileName.empty()) {
    mStream = new embedstream();
  }
  #endif

  if (!mStream) {
    auto *fileStream = new std::ifstream(mFileName, std::ios::binary);
    if (!fileStream->is_open())
    {
      Serial.printf("Failed to open file.\n");
      delete fileStream;
      return false;
    }
    mStream = fileStream;
  }

  // check the file is valid
  ChunkHeader header;
  // Read RIFF header
  readChunk(*mStream, &header);
  if (strncmp(header.chunkId, "RIFF", 4) != 0)
  {
    Serial.println("Not a valid AVI file.");
    delete mStream;
    mStream = nullptr;
    return false;
  }
  else
  {
    Serial.printf("RIFF header found.\n");
  }
  // next four bytes are the RIFF type which should be 'AVI '
  char riffType[4];
  mStream->read(riffType, 4);
  if (strncmp(riffType, "AVI ", 4) != 0)
  {
    Serial.println("Not a valid AVI file.");
    delete mStream;
    mStream = nullptr;
    return false;
  }
  else
  {
    Serial.println("RIFF Type is AVI.");
  }

  // now read each chunk and find the movi list
  while (mStream->good())
  {
    readChunk(*mStream, &header);
    if (mStream->eof() || mStream->fail())
    {
      break;
    }
    // is it a LIST chunk?
    if (strncmp(header.chunkId, "LIST", 4) == 0)
    {
      if (isMoviListChunk(header.chunkSize))
      {
        break;
      }
    }
    else
    {
      // skip the chunk data bytes
      mStream->seekg(header.chunkSize, std::ios::cur);
    }
  }
  // did we find the list?
  if (mMoviListPosition == 0)
  {
    Serial.printf("Failed to find the movi list.\n");
    delete mStream;
    mStream = nullptr;
    return false;
  }
  // keep the file open for reading the frames
  return true;
}

size_t AVIParser::getNextChunk(uint8_t **buffer, size_t &bufferLength)
{
  // check if the stream is open
  if (!mStream)
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
    readChunk(*mStream, &header);
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
      }
      // copy the chunk data
      mStream->read(reinterpret_cast<char *>(*buffer), header.chunkSize);
      mMoviListLength -= header.chunkSize;
      // handle any padding bytes
      if (header.chunkSize % 2 != 0)
      {
        mStream->seekg(1, std::ios::cur);
        mMoviListLength--;
      }
      return header.chunkSize;
    }
    else
    {
      // the data is not what was required - skip over the chunk
      mStream->seekg(header.chunkSize, std::ios::cur);
      mMoviListLength -= header.chunkSize;
    }
    // handle any padding bytes
    if (header.chunkSize % 2 != 0)
    {
      mStream->seekg(1, std::ios::cur);
      mMoviListLength--;
    }
  }
  // no more chunks
  Serial.println("No more data");
  return 0;
}
