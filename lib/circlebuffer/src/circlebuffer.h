#pragma once

/* circlebuffer library by Demetrius Bell
 */

// This will load the definition for common Particle variable types
#include "Particle.h"

class Circlebuffer {

public:

  Circlebuffer();
  bool alloc(unsigned int size);
  bool dealloc();
  
  uint8_t popByte();
  uint16_t popShort();

  void pushByte( uint8_t val );
  void pushBytes( uint8_t *vals, int len );

  void pushShort( uint16_t val );

  bool next();


private:
  uint8_t *_data;
  unsigned int bufsize;
  unsigned int readpos;
  unsigned int writepos;

};
