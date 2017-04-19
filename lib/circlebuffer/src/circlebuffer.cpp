/* circlebuffer library by Demetrius Bell
 */

#include "circlebuffer.h"
#include "stdio.h"
#include "stdint.h"

Circlebuffer::Circlebuffer(){
  // be sure not to call anything that requires hardware be initialized here, put those in begin()
    wrapped = false;
    bufsize = readpos = writepos = 0;
}

size_t Circlebuffer::getSize(){
  return this->bufsize;
}

uint8_t *Circlebuffer::getBuffer(int pos){
    return &this->_data[pos];
}

uint8_t *Circlebuffer::getWriteBuffer(){
    return this->getBuffer(this->writepos);
}


uint8_t *Circlebuffer::getReadBuffer(){
    return this->getBuffer(this->readpos);
}

int Circlebuffer::getReadAvailable(){
    if ( this->readpos > this->writepos ){
        return this->bufsize - this->readpos;
    }else if ( this->readpos < this->writepos ){
        return this->writepos - this->readpos;
    }else if ( this->wrapped && this->writepos == this->readpos ){
        return this->bufsize;
    }

    return 0;
}

int Circlebuffer::getWriteAvailable(){
    return this->bufsize - this->writepos;
}


int Circlebuffer::unreadByteCount(){
    if ( this->readpos == this->writepos ){
        return this->wrapped ? this->bufsize : 0;
    }else if ( this->readpos > this->writepos ){
        return ( this->bufsize-this->readpos)+this->writepos;
    }else {
        return this->writepos - this->readpos;
    }
}


int Circlebuffer::getReadpos(){
    return this->readpos;
}


int Circlebuffer::getWritepos(){
    return this->writepos;
}


bool Circlebuffer::alloc(size_t size){
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

bool Circlebuffer::moveWriteHead( int amount ){
    bool ret = false;
    this->writepos += amount;
    if ( this->writepos >= this->bufsize){
        this->writepos = 0;
        ret = true;
    }
    if ( this->writepos >= this->readpos ){
        this->wrapped = true;
    }
    return ret;
}

bool Circlebuffer::moveReadHead( int amount ){
    bool ret = false;
    this->readpos += amount;
    if ( this->readpos >= this->bufsize){
        this->readpos = 0;
        ret = true;
    }
    if ( this->readpos >= this->writepos ){
        this->wrapped = false;
    }
    return ret;
}

uint8_t Circlebuffer::popByte(){
    uint8_t val = this->_data[this->readpos];
    this->moveReadHead(1);
    return val;
}

uint16_t Circlebuffer::popShort(){
    if ( this->next() ){
        uint8_t ib = this->_data[this->readpos];
        uint8_t sb = this->_data[this->readpos+1];
        this->_data[this->readpos] = 0;
        this->_data[this->readpos+1] = 0;
        uint16_t val = ( sb << 8 ) + ib;
        this->moveReadHead(2);
        return val;
    }else {
        return 0;
    }
}

int16_t Circlebuffer::popSignedShort(){
    if ( this->next() ){
        uint8_t ib = this->_data[this->readpos];
        uint8_t sb = this->_data[this->readpos+1];
        this->_data[this->readpos] = 0;
        this->_data[this->readpos+1] = 0;
        int16_t val = ( sb << 8 ) + ib;
        this->moveReadHead(2);
        return val;
    }else {
        return 0;
    }
}


bool Circlebuffer::next(){
    return this->wrapped || this->readpos != this->writepos;
}


bool Circlebuffer::pushByte( uint8_t val ){
    this->_data[ this->writepos ] = val;
    this->writepos++;
    if ( this->writepos >= this->bufsize ){
        this->writepos = 0;
        if ( this->writepos == this->readpos ){
            this->wrapped = true;
            return true;
        }
    }
    return false;
}


bool Circlebuffer::pushShort ( uint16_t val ){
    this->_data[ this->writepos ] = val & 0xFF;
    this->_data[ this->writepos+1 ] = (val >> 8);

    this->writepos+=2;
    if ( this->writepos >= this->bufsize ){
        this->writepos = 0;
        if ( this->writepos == this->readpos ){
            this->wrapped = true;
            return true;
        }
    }

    return false;
}
