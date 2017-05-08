#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <SPI.h>
#include <circlebuffer.h>
#include <rawplayer.h>
#include <ztoyhub.h>

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
Ztoyhub *ztoy;

Circlebuffer speakerBuffer;
Circlebuffer micBuffer;

void setup() {

    Particle.disconnect();
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

    ztoy = new Ztoyhub();
    ztoy->begin( tcpsocket );

    player = new Rawplayer();
    player->begin( tcpsocket, speakerBuffer, SPEAKER_PIN );

    display.display();
    delay(250);

    display.clearDisplay();
    //setADCSampleTime(ADC_SampleTime_144Cycles);
    setADCSampleTime(ADC_SampleTime_84Cycles);
    player->turnOffSpeaker();

}

/*
 * for speach detection feature 
 */
unsigned long lastsample = 0;
unsigned long lastbeat = 0;
bool ok = false;

double silentThreshold = 0;
int pages = 0;

void loop(){

    if ( !ok ){

        if ( tcpsocket->available() > 0 ){
            ztoy->recieveHeader();
            if ( ztoy->getHeaderCode() == ZTOY_16BITAUDIO){
                Serial.print("16bit audio stream playback - ");
                display.clearDisplay();
                display.setTextSize(1);
                display.setTextColor(WHITE);
                display.setCursor(0,32);
                display.println(ztoy->getHeaderMessage());
                display.setCursor(0,0);
                display.println("Status: Idle");
                display.print("Volts: ");
                display.println( silentThreshold );
                display.display();
                if (player->recieveAndPlay()){
                    Serial.println("speaker off");
                    player->turnOffSpeaker();
                }else {
                    Serial.println("not found");
                }
            }else {
                Serial.println("unknown ztoyhub command");
            }
        }

        silentThreshold = getAmbiantNoiseAverage(50);
        ok = digitalRead(BTN1) == HIGH;

        if ( ok ){
            ztoy->clearMessage();
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(WHITE);
            display.setCursor(0,0);
            display.println("Status: Recording");
            display.display();
        }else {
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(WHITE);
            display.setCursor(0,32);
            display.println(ztoy->getHeaderMessage());
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
                        sendIotHubMessage(8);
                        ok = true;
                    }
                }else {
                    Serial.println("still connected");
                    ok = true;
                }
            }else {
                Serial.println("Stopped recording");
                display.clearDisplay();
                display.setTextSize(1);
                display.setTextColor(WHITE);
                display.setCursor(0,0);
                display.println("status: Waiting...");
                display.display();
            }
        }

        //if ( beat - lastsample > 125 ){
        if ( beat - lastsample > 62 ){
            if ( readMic16( &micBuffer ) ){
                sendAudio( &micBuffer );
            }
            lastsample = beat;
        }
    }

}

char *recieveHeader (){

    uint8_t header[10];
    tcpsocket->read(header, sizeof(header) );
    if ( header[0] == 0 && header[1] == 255 ){
        uint8_t ib = header[2];
        uint8_t sb = header[3];

        //size of character message after header
        uint16_t val = ( sb << 8 ) + ib;
        char  *msg  = new char[val];
        int idx = 0;
        while ( val > 0 ){
            uint8_t buf[10];
            int r = tcpsocket->read(buf,sizeof(buf));
            val-= r;

            for (int i = 0;i < r;i++){
                msg[idx++] = (char) buf[i];
            }
        }


        Serial.print("msg = ");
        Serial.println(msg);

        return msg;
    }

    return "";
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
    
    //uint16_t v1 = static_cast<uint16_t>( ads.readADC_SingleEnded(0) );
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

int sendIotHubMessage( uint8_t cmd ){

    char *msg = "testing";
    uint8_t buf[] = {0,255,cmd,0,0,0,0,0,0,0};
    unsigned int len = sizeof(msg);
    int sent = 0;

    buf[3] = len & 0xFF;
    buf[4] = (len >> 8);

    sent += tcpsocket->write( buf , sizeof(buf));
    sent += tcpsocket->println(msg);

    Serial.print("Sent ");
    Serial.print(sent);
    Serial.println(" bytes");
    return sent;

}


void sendAudio( Circlebuffer *buf) {
        writeTcpSocket( buf );
}

