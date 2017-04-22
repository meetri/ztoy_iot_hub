#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <SPI.h>
#include <circlebuffer.h>
#include <rawplayer.h>

#define MICROPHONE_PIN A5
#define MAX_PACKET_SIZE 1024
#define SPEAKER_BUFFER_SIZE 1024
#define MICROPHONE_BUFFER_SIZE 2048
#define LED1 D7
#define BTN1 D5
#define SPEAKER_PIN DAC1
#define SEND_OVER_TCP 1
#define BEATDELAY 500000

//======OLED=========
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);


IPAddress server = IPAddress(192,168,86,179);
int servicePort = 50050;

TCPClient *tcpsocket;
Rawplayer *player;

Circlebuffer speakerBuffer;
Circlebuffer micBuffer;


void setup() {
    Serial.begin(115200);

    pinMode(LED1,OUTPUT);
    pinMode(BTN1,INPUT_PULLDOWN);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

    Serial.println("Waiting for wifi...");
    while (!WiFi.ready()) Particle.process();

    Serial.println("allocating buffers");
    speakerBuffer.alloc( SPEAKER_BUFFER_SIZE );
    micBuffer.alloc( MICROPHONE_BUFFER_SIZE );

    tcpsocket = new TCPClient();
    player = new Rawplayer();
    player->begin( tcpsocket, speakerBuffer, SPEAKER_PIN );

    display.display();
    delay(250);

    display.clearDisplay();

}

/*
 * for speach detection feature 
 */
unsigned long lastsample = 0;
unsigned long lastbeat = 0;
bool ok = false;

int ms = 125;
double silentThreshold = 0;
int pages = 0;

void loop(){

    if ( !ok ){

        if ( tcpsocket->available() > 0 ){
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(WHITE);
            display.setCursor(0,0);
            display.println("Status: Playing Stream");
            display.display();
            if (player->recieveAndPlay()){
                digitalWrite(SPEAKER_PIN,0);
            }
        }

        silentThreshold = getAmbiantNoiseAverage(50);
        ok = digitalRead(BTN1) == HIGH;

        if ( ok ){
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(WHITE);
            display.setCursor(0,0);
            display.println("Status: Stream Recording");
            display.display();
        }else {
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(WHITE);
            display.setCursor(0,0);
            display.println("Status: Idle");
            display.print("Volts: ");
            display.println( silentThreshold );
            display.display();
        }
        
    }

    while ( ok ) {

        unsigned long beat = micros();
        if ( beat - lastbeat > BEATDELAY ){
            ok = false;
            lastbeat = beat;
            if ( WiFi.ready() && digitalRead(BTN1) == HIGH ) {
            //if ( WiFi.ready() && threshold >= 0.1 ) {
                if ( !tcpsocket->connected() ){
                    if ( tcpsocket->connect( server , servicePort ) ){
                        Serial.println("connected");
                        ok = true;
                    }
                }else {
                    Serial.println("still connected");
                    ok = true;
                }
            }else {
                Serial.println("Stopped recording");
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

double getAmbiantNoiseAverage(unsigned int sampleWindow){

    unsigned long startMillis= millis();  // Start of sample window
    unsigned int peakToPeak = 0;   // peak-to-peak level

    unsigned int signalMax = 0;
    unsigned int signalMin = 4096;

    unsigned int sample;

    // collect data for 50 mS
    while (millis() - startMillis < sampleWindow)
    {
        sample = analogRead(MICROPHONE_PIN);
        if (sample < 4096)  // toss out spurious readings
        {
            if (sample > signalMax)
            {
                signalMax = sample;  // save just the max levels
            }
            else if (sample < signalMin)
            {
                signalMin = sample;  // save just the min levels
            }
        }
    }
    peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
//double volts = (peakToPeak * 5.0) / 1024;  // convert to volts
    double volts = (peakToPeak * 5.0) / 4096;  // convert to volts
    
    return volts;
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

void writeTcpSocket ( Circlebuffer *buf){
    while ( buf->next() ){
        int r = tcpsocket->write( buf->getReadBuffer() , buf->getReadAvailable());
        buf->moveReadHead(r);
    }
}

void sendAudio( Circlebuffer *buf) {
        writeTcpSocket( buf );
}

