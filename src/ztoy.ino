#define MICROPHONE_PIN A5
#define AUDIO_INPUT_BUFFER_MAX 2048
#define AUDIO_BUFFER_MAX 1024
#define LED1 D7
#define BTN1 D1
#define SPEAKER_PIN DAC1

//#include "circlebuffer.h"

IPAddress server = IPAddress(192,168,86,179);
int servicePort = 50050;

UDP *socket;

//Stream In
uint8_t udpInBuffer[AUDIO_INPUT_BUFFER_MAX+2];
int lastWritePos = 0,lastReadPos = 0;
unsigned long lastRead;


//Stream Out
uint8_t udpBuffer[AUDIO_BUFFER_MAX];
unsigned long stopFrame = 0;
int audioIdx = 0;
unsigned long lastReadOut;


void setup() {
    Serial.begin(115200);
    //pinMode(MICROPHONE_PIN, INPUT);
    pinMode(LED1,OUTPUT);
    pinMode(BTN1,INPUT_PULLDOWN);
    pinMode(SPEAKER_PIN,OUTPUT);
    
    socket = new UDP();
    
    //while(!Serial.available()) Particle.process();
    
    while (!WiFi.ready()) Particle.process();
    socket->begin( servicePort );
    if (!socket->setBuffer(1024)){
        Serial.println("problem setting socket buffer");
    }
    
    lastRead = micros();
    lastReadOut = micros();
    
    
}


void loop() {
    
    if (WiFi.ready()) {
        
        recieveFromUdp();
        playBuffer();
        
        if ( digitalRead(BTN1) == HIGH ){
            digitalWrite(LED1,HIGH);
            Serial.println("generating 100ms audio frame");
            listenAndSend(100);
            stopFrame = millis() + 100;
        }else {
            digitalWrite(LED1,LOW);
            if ( stopFrame > 0 && millis() > stopFrame ){
                stopFrame = 0;
                sendStopFrame();
            }
        }
        
    }
    
}

void playBuffer() {
    
    bool b = false;
    while ( lastReadPos != lastWritePos ){
        b = true;
        if ( lastReadPos >= AUDIO_INPUT_BUFFER_MAX){
            lastReadPos = 0;
        }
        
        unsigned long time = micros();
        if (lastRead > time) {
            // time wrapped? how does this happen?
            //lets just skip a beat for now, whatever.
            Serial.println("time quake - time just looped over itself.");
            lastRead = time;
        }
        
        //125 microseconds is 1/8000th of a second
        if ((time - lastRead) > 125) {
            lastRead = time;
            lastReadPos += play16BitFrame( lastReadPos );
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

int play16BitFrame(int pos){
    uint8_t b1 = udpInBuffer[pos];
    uint8_t b2 = udpInBuffer[pos+1];
    int val = (b2 << 8) + b1;
    val = map(val,0,65535,0,4095);
    analogWrite(SPEAKER_PIN, val );
    return 2; 
}

int play8BitFrame( int pos ){
    analogWrite(SPEAKER_PIN, map(udpInBuffer[lastReadPos],0,255,0,4095) );
    return 1; 
}

void recieveFromUdp(){
    
    if ( socket->parsePacket() ){
        
        int avail = socket->available();
        if ( avail > 0 ){
            
            if ( lastWritePos >= AUDIO_INPUT_BUFFER_MAX ) {
                lastWritePos = 0;
            }
            
            int buflen = AUDIO_INPUT_BUFFER_MAX - lastWritePos;
            if (avail < buflen ){
                buflen = avail;
            }
            
            int received = socket->read( &udpInBuffer[lastWritePos], buflen );
            //Serial.print("Received ");
            //Serial.println(received);
            lastWritePos += received;
            
        }
        
    }
    
}

// ------------------------------------------------------------------------------------------
// STREAM FROM MICROPHONE
// ---------------------------------

void listenAndSend(int delay) {
    bool full = false;
    while ( !full ){
        
        unsigned long time = micros();
        if (lastReadOut > time) {
            // time wrapped? how does this happen?
            //lets just skip a beat for now, whatever.
            Serial.println("time quake - time just looped over itself.");
            lastReadOut = time;
        }
        
        //125 microseconds is 1/8000th of a second
        if ((time - lastReadOut) > 125) {
            lastReadOut = time;
            full = readMic16();
        }
        
    }
    
    sendAudio();
    
}


bool readMic16(void) {
    uint16_t value = map(analogRead(MICROPHONE_PIN),0,4096,0,65535);
    udpBuffer[audioIdx] = value & 0xff;
    udpBuffer[audioIdx+1] = (value >> 8);
    audioIdx+=2;
    return audioIdx >= AUDIO_BUFFER_MAX;
}

bool readMic(void) {
    udpBuffer[audioIdx++] = map(analogRead(MICROPHONE_PIN),0,4096,0,255);
    return audioIdx >= AUDIO_BUFFER_MAX;
}

void sendStopFrame(void){
    memset(udpBuffer, '\0', AUDIO_BUFFER_MAX );
    int sent = socket->sendPacket(udpBuffer,100,server,servicePort);
}

// Callback for Timer 1
void sendAudio(void) {
    writeSocket(udpBuffer);
}

void writeSocket(uint8_t *buffer) {
    int sent = socket->sendPacket(buffer,audioIdx,server,servicePort);
    audioIdx = 0;
}
    

    
