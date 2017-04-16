#include <circlebuffer.h>

#define MICROPHONE_PIN A5
#define AUDIO_INPUT_BUFFER_MAX 256
#define AUDIO_BUFFER_MAX 4096
#define LED1 D7
#define BTN1 D1
#define SPEAKER_PIN DAC1

TCPClient *tcpsocket;
IPAddress server = IPAddress(192,168,86,179);
int servicePort = 50050;

Circlebuffer speakerBuffer;
Circlebuffer micBuffer;

unsigned long lastRead;
unsigned long lastReadOut;

void setup() {
    Serial.begin(115200);

    pinMode(LED1,OUTPUT);
    pinMode(BTN1,INPUT_PULLDOWN);
    pinMode(SPEAKER_PIN,OUTPUT);

    tcpsocket = new TCPClient();

    while (!WiFi.ready()) Particle.process();

    speakerBuffer.alloc( AUDIO_INPUT_BUFFER_MAX );
    micBuffer.alloc( AUDIO_BUFFER_MAX );

    lastRead = micros();
    lastReadOut = micros();

}


void loop() {

    if (WiFi.ready()) {

        if ( digitalRead(BTN1) == HIGH ){

            digitalWrite(LED1,HIGH);
            if ( !tcpsocket->connected() ){
                Serial.print("*");
                tcpsocket->connect( server , servicePort );
            }    

            Serial.print(".");
            listenAndSend(200,&micBuffer);

        }else {

            digitalWrite(LED1,LOW);
            recieveAndPlay( &speakerBuffer); 

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
            play8BitFrame(buf);
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

void recieveAndPlay( Circlebuffer *buf ){

    bool ret = false;
    int avail = tcpsocket->available();
    while ( avail > 0 ){
        ret = true;
        int buflen = buf->getWriteAvailable();
        if (avail < buflen ){
            buflen = avail;
        }

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

// ------------------------------------------------------------------------------------------
// STREAM FROM MICROPHONE
// ---------------------------------

void listenAndSend(int delay, Circlebuffer *buf ) {
    int numframes = delay * 1000 / 125 ;
    int frames = 0;
    int dif = 0;
    uint8_t lv = 0;
    while ( frames < numframes ){

        unsigned long time = micros();
        if (lastReadOut > time) {
            lastReadOut = time;
        }

        //125 microseconds is 1/8000th of a second
        int dif = time-lastReadOut;
        if ( dif < 125 ){
            delayMicroseconds(125-dif);
        }

        lastReadOut = micros();
        if ( readMic(buf) ){
            sendAudio(buf);
        }

        frames++;

    }
    
    sendAudio(buf);

}


bool readMic16(Circlebuffer *buf) {
    uint16_t value = map(analogRead(MICROPHONE_PIN),0,4096,0,65535);
    return buf->pushShort( value );
}

uint8_t readMic(Circlebuffer *buf) {
    uint8_t value = map(analogRead(MICROPHONE_PIN),0,4096,0,255);
    return buf->pushByte( value );
}

void writeTcpSocket ( Circlebuffer *buf ){

    if (tcpsocket->connected() ){
        int written = 0;
        while ( buf->next() ){
            int r = tcpsocket->write( buf->getReadBuffer() , buf->getReadAvailable() );
            buf->moveReadHead(r);
            written += r;
        }
    }
}

int sendAudioFrame( Circlebuffer *buf, int numbytes ) {
    if (tcpsocket->connected() ){
        if ( buf->next() ){
            buf->moveReadHead(tcpsocket->write( buf->getReadBuffer() , numbytes ));
            return numbytes;
        }
    }
}

void sendAudio( Circlebuffer *buf) {
    writeTcpSocket( buf );
}

