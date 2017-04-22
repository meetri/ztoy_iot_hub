/* ztoyhub library by demetrius bell
 */

#include "ztoyhub.h"

Ztoyhub::Ztoyhub()
{
  // be sure not to call anything that requires hardware be initialized here, put those in begin()
}

void Ztoyhub::begin( TCPClient *client ) {
    Serial.println("called begin");
    this->client = client;
}


uint16_t Ztoyhub::getHeaderCode(){
    return this->header_msgcode;
}

char *Ztoyhub::getHeaderMessage(){
    return this->header_message;
}


void Ztoyhub::recieveHeader()
{
    uint8_t header[10];
    this->client->read(header, sizeof(header) );
    if ( header[0] == 0 && header[1] == 255 ){
        Serial.print("Header valid - ");
        uint8_t ib = header[2];
        uint8_t sb = header[3];
        uint16_t val = ( sb << 8 ) + ib;

        Serial.print("message size - ");
        Serial.print(val);
        ib = header[4];
        sb = header[5];
        this->header_msgcode = ( sb << 8 ) + ib;

        Serial.print(" message code - ");
        Serial.print(this->header_msgcode);

        //char msg[100];
        uint8_t *inmsg = new uint8_t[val];
        char *msg = new char[val+1];

        int idx = 0;
        this->client->read(inmsg,val);
        for ( int i = 0; i < val;i++){
            msg[i] = (char) inmsg[i];
        }
        msg[val] = NULL;

        Serial.print(" message - [");
        Serial.print(msg);
        Serial.print("] length = ");
        Serial.println(val);



        this->header_message = msg;
    }
}

