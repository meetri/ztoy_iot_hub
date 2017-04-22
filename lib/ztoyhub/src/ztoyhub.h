#pragma once

#define ZTOY_16BITAUDIO 100

/* ztoyhub library by demetrius bell
 */

#include "Particle.h"

class Ztoyhub
{
public:
  Ztoyhub();

  void begin( TCPClient *client );
  void recieveHeader();
  uint16_t getHeaderCode();
  char *getHeaderMessage();

private:
  TCPClient *client;
  char *header_message;
  uint16_t header_msgcode;
};
