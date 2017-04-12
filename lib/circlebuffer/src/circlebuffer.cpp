/* circlebuffer library by Demetrius Bell
 */

#include "circlebuffer.h"

Circlebuffer::Circlebuffer(){
  // be sure not to call anything that requires hardware be initialized here, put those in begin()
}

bool Circlebuffer::alloc(unsigned int size){
    this->dealloc();

    if ( size % 2 == 0 ) {
        this->_data = new uint8_t[size];
        this->bufsize = size;
    }

    return this->_data != 0;
}

bool Circlebuffer::dealloc(){

    if ( this->_data == 0 ){
        delete[] this->_data;
        this->bufsize = 0;
        return true;
    }
    return false;
}

uint8_t Circlebuffer::popByte(){
    if ( this->next() ){
        uint8_t val = this->_data[this->readpos];
        this->_data[this->readpos] = 0;
        this->readpos++;
        if ( this->readpos >= this->bufsize){
            this->readpos = 0;
        }
        return val;
    }else {
        return 0;
    }
}

uint16_t Circlebuffer::popShort(){
    if ( this->next() ){
        uint8_t sb = this->_data[this->readpos];
        uint8_t ib = this->_data[this->readpos+1];
        this->_data[this->readpos] = 0;
        this->_data[this->readpos+1] = 0;
        uint16_t val = ( sb << 8 ) + ib;
        this->readpos=2;
        if ( this->readpos >= this->bufsize){
            this->readpos = 0;
        }
        return val;
    }else {
        return 0;
    }
}

bool Circlebuffer::next(){
    return this->readpos != this->writepos;
}

void Circlebuffer::pushByte( uint8_t val ){
    this->_data[ this->writepos ] = val;
    this->writepos++;
    if ( this->writepos >= this->bufsize ){
        this->writepos = 0;
    }
}

void Circlebuffer::pushBytes( uint8_t *vals , int len){

    for ( int i = 0; i < len;i++ ){
        pushByte( vals[i] );
    }

}

void Circlebuffer::pushShort ( uint16_t val ){
    this->_data[ this->writepos ] = val & 0xFF;
    this->_data[ this->writepos+1 ] = (val >> 8);

    this->writepos+=2;
    if ( this->writepos >= this->bufsize ){
        this->writepos = 0;
    }
}
