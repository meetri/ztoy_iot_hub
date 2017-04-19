#include <circlebuffer.h>

#define MICROPHONE_PIN A5
#define MAX_PACKET_SIZE 1024
#define SPEAKER_BUFFER_SIZE 1024
#define MICROPHONE_BUFFER_SIZE 2048
#define LED1 D7
#define BTN1 D1
#define SPEAKER_PIN DAC1
#define SEND_OVER_TCP 1
#define BEATDELAY 500000

UDP *udpsocket;
TCPClient *tcpsocket;

IPAddress server = IPAddress(192,168,86,179);
int servicePort = 50050;

Circlebuffer speakerBuffer;
Circlebuffer micBuffer;

unsigned long lastRead;
unsigned long lastReadOut;

unsigned long stopFrame;

void setup() {
    Serial.begin(115200);

    pinMode(LED1,OUTPUT);
    pinMode(BTN1,INPUT_PULLDOWN);
    pinMode(SPEAKER_PIN,OUTPUT);

    tcpsocket = new TCPClient();
    while (!WiFi.ready()) Particle.process();

    speakerBuffer.alloc( SPEAKER_BUFFER_SIZE );
    micBuffer.alloc( MICROPHONE_BUFFER_SIZE );

    lastRead = micros();
    lastReadOut = micros();

}

unsigned long lastsample = 0;
unsigned long lastbeat = 0;
bool ok = false;

int ms = 125;

void loop(){

    if ( !ok ){
        ok = digitalRead(BTN1) == HIGH;
        recieveAndPlay( &speakerBuffer );
    }

    while ( ok ) {

        unsigned long beat = micros();

        if ( beat - lastbeat > BEATDELAY ){
            ok = false;
            lastbeat = beat;
            if ( WiFi.ready() && digitalRead(BTN1) == HIGH ) {
                if ( !tcpsocket->connected() ){
                    if ( tcpsocket->connect( server , servicePort ) ){
                        Serial.println("connected");
                        ok = true;
                    }
                }else {
                    Serial.println("still connected");
                    ok = true;
                }
            }
        }

        if ( beat - lastsample > 125 ){
            if ( readMic16( &micBuffer ) ){
                sendAudio( &micBuffer );
            }
            lastsample = beat;
        }
    }


}

void playBuffer( Circlebuffer *buf ) {

    bool b = false;
    while ( buf->next() ){
        b = true;
        unsigned long time = micros();
        if (lastRead > time) {
            // time wrapped? how does this happen?
            //lets just skip a beat for now, whatever.
            Serial.println("time quake - time just looped over itself.");
            lastRead = time;
        }

        //125 microseconds is 1/8000th of a second
        int dif = time-lastRead;
        if ( dif >= 125) {
            lastRead = time;
            //play8BitFrame(buf);
            playUnsigned16BitFrame(buf);
        }

    }

    if ( b ){
        Serial.println("Played audio sample");
    }

    //shut off speaker...
    if ( micros() - lastRead > 1000 ){
        analogWrite(SPEAKER_PIN, 0 );
    }

}

int playUnsigned16BitFrame( Circlebuffer *buf ){
    int val = buf->popSignedShort();
    val = map(val,-32768,32768,0,4095);
    analogWrite(SPEAKER_PIN, val );
    return 2;
}

int play16BitFrame( Circlebuffer *buf ){
    int val = buf->popShort();
    val = map(val,0,65535,0,4095);
    analogWrite(SPEAKER_PIN, val );
    return 2;
}

int play8BitFrame(Circlebuffer *buf){
    uint8_t val = buf->popByte();
    analogWrite(SPEAKER_PIN, map(val,0,255,0,4095) );
    return 1;
}


void recieveAndPlayUDP( Circlebuffer *buf ){

   while ( udpsocket->parsePacket() ){
        int avail = udpsocket->available();
        if ( avail > 0 ){

            int buflen = buf->getWriteAvailable();
            int received = udpsocket->read( buf->getWriteBuffer(), buflen );
            if ( buf->moveWriteHead(received) ){
                playBuffer( buf );
            }
            Serial.print("Received ");
            Serial.println(received);
        }
    }

    playBuffer( buf );

}

void recieveAndPlayTCP( Circlebuffer *buf ){
    int avail = tcpsocket->available();
    while ( avail > 0 ){
        int buflen = buf->getWriteAvailable();
        int received = tcpsocket->read( buf->getWriteBuffer(), buflen );
        if ( buf->moveWriteHead(received) ){
            playBuffer( buf );
        }
        Serial.print("Received ");
        Serial.println(received);
        avail = tcpsocket->available();
    }

    playBuffer( buf );
}

void recieveAndPlay( Circlebuffer *buf ){

    if ( SEND_OVER_TCP == 1 ){
        recieveAndPlayTCP(buf);
    }else {
        recieveAndPlayUDP(buf);
    }

}

// ------------------------------------------------------------------------------------------
// STREAM FROM MICROPHONE
// ---------------------------------

bool readMic16(Circlebuffer *buf) {
    return buf->pushShort( map(analogRead(MICROPHONE_PIN),0,4096,0,65535) );

}

uint8_t readMic(Circlebuffer *buf) {
    return buf->pushByte( map(analogRead(MICROPHONE_PIN),0,4096,0,255) );
}

void writeTcpSocket ( Circlebuffer *buf ){
    while ( buf->next() ){
        int r = tcpsocket->write( buf->getReadBuffer() , buf->getReadAvailable());
        buf->moveReadHead(r);
    }
}

void writeUdpSocket ( Circlebuffer *buf ){

    int written = 0;
    bool sockopen = false;
    while ( buf->next() ){

        if ( sockopen == false ){
            udpsocket->beginPacket( server, servicePort );
            sockopen = true;
            written = 0;
        }

        int r = udpsocket->write( buf->getReadBuffer() , buf->getReadAvailable() );
        buf->moveReadHead(r);
        written += r;

        if ( written == MAX_PACKET_SIZE ){
            udpsocket->endPacket();
            sockopen = false;
        }

    }

    if ( sockopen == true ){
        udpsocket->endPacket();
        sockopen = false;
    }

}

void sendAudio( Circlebuffer *buf) {
    if ( SEND_OVER_TCP == 1 ){
        writeTcpSocket( buf );
    }else {
        writeUdpSocket( buf );
    }
}


void sendStopFrame(Circlebuffer *buf){
    udpsocket->sendPacket(buf->getBuffer(0),100,server,servicePort);
}
