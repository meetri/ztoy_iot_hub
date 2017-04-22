#pragma once

/* rawplayer library by demetrius bell
 */

#include <circlebuffer.h>
#include "Particle.h"

class Rawplayer
{
public:
  Rawplayer();

  void begin( TCPClient *tcpclient, Circlebuffer buf, int speakerpin );
  bool playBuffer();
  void sendAudio();
  bool recieveAndPlay();

  int playSigned16BitFrame();
  int play16BitFrame();
  int play8BitFrame();

private:
  int speakerPin;
  unsigned long lastRead;
  Circlebuffer buffer;
  TCPClient *client;
};
