// Example usage for ztoyhub library by demetrius bell.

#include "ztoyhub.h"

// Initialize objects from the lib
Ztoyhub ztoyhub;

void setup() {
    // Call functions on initialized library objects that require hardware
    ztoyhub.begin();
}

void loop() {
    // Use the library's initialized objects and functions
    ztoyhub.process();
}
