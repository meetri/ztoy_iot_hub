/* rawplayer library by demetrius bell
 */

#include "rawplayer.h"

Rawplayer::Rawplayer()
{
  // be sure not to call anything that requires hardware be initialized here, put those in begin()
}


void Rawplayer::begin( TCPClient *tcpclient , Circlebuffer buf, int speakerpin ){
    // initialize hardware
    this->buffer = buf;
    this->speakerPin = speakerpin;
    this->client = tcpclient;
    this->lastRead = micros();

    pinMode(this->speakerPin,OUTPUT);
    Serial.println("rawplay initialized");
}


bool Rawplayer::playBuffer() {
    bool b = false;
    while ( this->buffer.next() ){
        b = true;
        unsigned long time = micros();
        if (this->lastRead > time) {
            // time wrapped? how does this happen?
            //lets just skip a beat for now, whatever.
            Serial.println("time quake - time just looped over itself.");
            this->lastRead = time;
        }

        //125 microseconds is 1/8000th of a second
        int dif = time-this->lastRead;
        if ( dif >= 125) {
            this->lastRead = time;
            //play8BitFrame(buf);
            this->play16BitFrame();
        }

    }

    if ( b ){
        return true;
    }

    return false;

}


int Rawplayer::playSigned16BitFrame(){
    int val = this->buffer.popSignedShort();
    val = map(val,-32768,32768,0,4095);
    analogWrite(this->speakerPin, val );
    return 2;
}


int Rawplayer::play16BitFrame(){
    int val = this->buffer.popShort();
    val = map(val,0,65535,0,4095);
    analogWrite(this->speakerPin, val );
    return 2;
}


int Rawplayer::play8BitFrame(){
    uint8_t val = this->buffer.popByte();
    analogWrite(this->speakerPin, map(val,0,255,0,4095) );
    return 1;
}

void Rawplayer::turnOffSpeaker(){
    analogWrite(this->speakerPin,0);
}


bool Rawplayer::recieveAndPlay(){
    bool ret = false;
    int avail = this->client->available();
    
    Serial.println("recieve and play");
    while ( avail > 0 ){
        ret = true;
        int buflen = this->buffer.getWriteAvailable();
        int received = this->client->read( this->buffer.getWriteBuffer(), buflen );
        if ( this->buffer.moveWriteHead(received) ){
            this->playBuffer();
        }
        avail = this->client->available();
    }

    this->playBuffer();
    return ret;
}




