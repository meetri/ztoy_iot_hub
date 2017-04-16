#include <circlebuffer.h>

#define MICROPHONE_PIN A5
#define AUDIO_INPUT_BUFFER_MAX 512
#define AUDIO_BUFFER_MAX 2048
#define MAX_PACKET_SIZE 1024
#define LED1 D7
#define BTN1 D1
#define SPEAKER_PIN DAC1


IPAddress server = IPAddress(192,168,86,179);
int servicePort = 50050;

UDP *socket;
TCPClient *tcpsocket;

//Stream In
Circlebuffer speakerBuffer;
Circlebuffer micBuffer;
unsigned long lastRead;


//Stream Out
/*
uint8_t udpBuffer[AUDIO_BUFFER_MAX];
int audioIdx = 0;
*/
unsigned long stopFrame = 0;
unsigned long lastReadOut;


void setup() {
    Serial.begin(115200);
    //pinMode(MICROPHONE_PIN, INPUT);
    pinMode(LED1,OUTPUT);
    pinMode(BTN1,INPUT_PULLDOWN);
    pinMode(SPEAKER_PIN,OUTPUT);

    socket = new UDP();
    tcpsocket = new TCPClient();


    //while(!Serial.available()) Particle.process();

    while (!WiFi.ready()) Particle.process();

    socket->begin( servicePort );
    if (!socket->setBuffer(MAX_PACKET_SIZE)){
        Serial.println("problem setting socket buffer");
    }

    speakerBuffer.alloc( AUDIO_INPUT_BUFFER_MAX );
    micBuffer.alloc( AUDIO_BUFFER_MAX );

    lastRead = micros();
    lastReadOut = micros();

}


void loop() {

    if (WiFi.ready()) {

        if ( digitalRead(BTN1) == HIGH ){
            if ( !tcpsocket->connected() ){
                Serial.println("Attempting to create TCP connection");
                tcpsocket->connect( server , servicePort );
            }    
            digitalWrite(LED1,HIGH);
            Serial.println("generating 100ms audio frame");
            listenAndSend(200,&micBuffer);
            stopFrame = millis() + 100;
        }else {
            if ( tcpsocket->connected() ){
                Serial.println("disconnecting tcp connection");
                tcpsocket->stop();
            }    
            digitalWrite(LED1,LOW);
            if ( stopFrame > 0 && millis() > stopFrame ){
                stopFrame = 0;
                Serial.println("sending stop frame");
                sendStopFrame(&micBuffer);
            }

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

    while ( socket->parsePacket() ){

        int avail = socket->available();
        if ( avail > 0 ){

            int buflen = buf->getWriteAvailable();
            if (avail < buflen ){
                buflen = avail;
            }

            int received = socket->read( buf->getWriteBuffer(), buflen );
            if ( buf->moveWriteHead(received) ){
                playBuffer( buf );
            }
            Serial.print("Received ");
            Serial.println(received);
        }

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
    while ( frames < numframes ){

        unsigned long time = micros();
        if (lastReadOut > time) {
            // time wrapped? how does this happen?
            //lets just skip a beat for now, whatever.
            Serial.println("time quake - time just looped over itself.");
            lastReadOut = time;
        }

        //125 microseconds is 1/8000th of a second
        int dif = time-lastReadOut;
        if ( dif >= 125) {
            lastReadOut = time;
            if ( readMic(buf) ) {
                sendAudio(buf);
            }
            frames++;
        }

    }

    sendAudio(buf);

}


bool readMic16(Circlebuffer *buf) {
    uint16_t value = map(analogRead(MICROPHONE_PIN),0,4096,0,65535);
    return buf->pushShort( value );
}

bool readMic(Circlebuffer *buf) {
    uint8_t value = map(analogRead(MICROPHONE_PIN),0,4096,0,255);
    return buf->pushByte( value );
}

void sendStopFrame(Circlebuffer *buf){
    socket->sendPacket(buf->getBuffer(0),100,server,servicePort);
}

void writeTcpSocket ( Circlebuffer *buf ){

    if (tcpsocket->connected() ){
        int written = 0;
        while ( buf->next() ){
            int r = tcpsocket->write( buf->getReadBuffer() , buf->getReadAvailable() );
            buf->moveReadHead(r);
            written += r;
        }
        Serial.print("Wrote ");
        Serial.print(written);
        Serial.println(" to TCP");
    }else {
        Serial.print("Unable to establish TCP connection");
    }
}

void writeSocket( Circlebuffer *buf ) {

    int written = 0;
    bool sockopen = false;
    while ( buf->next() ){
        if ( sockopen == false ){
            socket->beginPacket( server, servicePort );
            sockopen = true;
            written = 0;
        }
        written += socket->write( buf->getReadBuffer() ,1);
        buf->moveReadHead(1);
        if ( written == MAX_PACKET_SIZE ){
            socket->endPacket();
            sockopen = false;
        }

    }
    if ( sockopen == true ){
        socket->endPacket();
        sockopen = false;
    }

}


// Callback for Timer 1
void sendAudio( Circlebuffer *buf) {
    //writeSocket( buf );
    writeTcpSocket( buf );
}

