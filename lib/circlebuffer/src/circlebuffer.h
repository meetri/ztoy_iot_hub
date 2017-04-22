#pragma once

/* circlebuffer library by Demetrius Bell
 */

// This will load the definition for common Particle variable types
//#include "Particle.h"
#include <stdint.h>
#include <stdio.h>

class Circlebuffer {

public:

  Circlebuffer();
  bool alloc(size_t size);
  bool dealloc();
  
  uint8_t popByte();
  uint16_t popShort();
  int16_t popSignedShort();

  bool pushByte( uint8_t val );
  bool pushShort( uint16_t val );

  bool next();
  int getReadpos();
  int getWritepos();

  int getWriteAvailable();
  int getReadAvailable();
  int unreadByteCount();
  size_t getSize();

  uint8_t *getBuffer(int pos);
  uint8_t *getWriteBuffer();
  uint8_t *getReadBuffer();

  bool moveReadHead(int amount);
  bool moveWriteHead(int amount);

private:
  uint8_t *_data;
  bool wrapped;
  unsigned int bufsize;
  unsigned int readpos;
  unsigned int writepos;

};
