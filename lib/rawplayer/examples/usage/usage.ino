// Example usage for rawplayer library by demetrius bell.

#include "rawplayer.h"

// Initialize objects from the lib
Rawplayer rawplayer;

void setup() {
    // Call functions on initialized library objects that require hardware
    rawplayer.begin();
}

void loop() {
    // Use the library's initialized objects and functions
    rawplayer.process();
}
