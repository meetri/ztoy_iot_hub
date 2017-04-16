#include "circlebuffer.h"
#include <iostream>

void writeSomething(Circlebuffer *buf, int numBytes ){

    for ( int i = 0; i < numBytes;i++){
        buf->pushShort(100);
    }

    std::cout << "writesomething: read == " << buf->getReadpos() << " write == " << buf->getWritepos() << std::endl;
}

int readSomething(Circlebuffer *buf){

    int count = 0;

    while ( buf->next() ){
        buf->moveReadHead(1);
        count++;
    }

    std::cout << "readsomething: count == " << count << " read == " << buf->getReadpos() << " write == " << buf->getWritepos() << std::endl;

    return count;
}

bool test1(){

    uint16_t size = 200;
    Circlebuffer buf;
    buf.alloc(size);

    writeSomething(&buf,50);
    readSomething(&buf);


    return true;

}

int main(){

    test1();
    std::cout << "\nTest1 completed\n";

    return 0;
}
