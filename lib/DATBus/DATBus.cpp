#include "Arduino.h"
#include <DATBus.h>
#include <DATBusPins.h>

uint8_t dat_outputs[] = {DAT1, DAT2, DAT3, DAT4, DAT5, DAT6, DAT7, DAT8, DAT9, DAT10, DAT11, DAT12, DAT13, DAT14};

void setupPins(void) {
    pinMode(DAT0, INPUT);
    pinMode(DAT1, OUTPUT);
    pinMode(DAT2, OUTPUT);
    pinMode(DAT3, OUTPUT);
    pinMode(DAT4, OUTPUT);
    pinMode(DAT5, OUTPUT);
    pinMode(DAT6, OUTPUT);
    pinMode(DAT7, OUTPUT);
    pinMode(DAT8, OUTPUT);
    pinMode(DAT9, OUTPUT);
    pinMode(DAT10, OUTPUT);
    pinMode(DAT11, OUTPUT);
    pinMode(DAT12, OUTPUT);
    pinMode(DAT13, OUTPUT);
    pinMode(DAT14, OUTPUT);
}

// sends a byte,address, and bank to the DAT bus
// value : 0-255
// address : 0-16
// bank : 0-1 (0 = A = DAT13, 1 = B = DAT14)
void writeDAT(uint8_t value, uint8_t address, uint8_t bank) {
    uint8_t word[] = {0,0,0,0,0,0,0,0,0,0,0,0};
    // do the byte
    for(int i = 0; i < 8; i++) {
        word[i] = uint8_t(((0b10000000 >> i) & value) > 0); // convert to binary
    }
    // do the address
    for(int i = 0; i < 4; i++) {
        word[i+8] = uint8_t(((0b1000 >> i) & address) > 0); // convert to binary
    }
    // start outputting!
    // setup value and address outputs
    for(int i = 0; i < sizeof(word); i++) {
        digitalWrite(dat_outputs[i], word[i]);
    }
    // wait for the circuit to catch up
    delayMicroseconds(2);
    
    // latch the word with the bank signal
    digitalWrite(bank==0?DAT13:DAT14, HIGH);
    delayMicroseconds(3);
    digitalWrite(bank==0?DAT13:DAT14, LOW);
    delayMicroseconds(3);
}
