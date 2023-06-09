#include "Arduino.h"
#include <DATBus.h>
#include <DATBusPins.h>
#include <queue>

#define MAX_QUEUE_LEN 100

uint8_t datLines[] = {DAT1, DAT2, DAT3, DAT4, DAT5, DAT6, DAT7, DAT8, DAT9, DAT10, DAT11, DAT12, DAT13, DAT14};

uint8_t dspSettings[16][2]; //first index is address, second index is bank

uint8_t typicalPacketAddresses[] = {9, 11, 3, 5, 1, 8, 7, 7, 0, 5, 11, 4, 13, 13, 0, 8, 12, 4, 2, 12};
uint8_t typicalPacketBanks[] = {1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1};
uint8_t typicalPacketDefaultValues[] = {68,149,184,103,38,174,64,130,20,255,0,0,247,0,0,60,47,147,25,14};

uint16_t busWord = 0; // temporarily stores the word being read from the bus during an isr

std::queue<uint16_t> busQueue;

size_t getBusQueueSize() {
    return busQueue.size();
}

uint16_t getBusQueueFront() {
    return busQueue.front();
}

bool busQueueEmpty() {
    return busQueue.empty();
}

void popBusQueue() {
    busQueue.pop();
}

// pushes to the queue if length is less than MAX_QUEUE_LEN.
// if full, pops the oldest value from the queue before adding new one
void tryPush(uint16_t word, std::queue<uint16_t>& q) {
    if (q.size() >= MAX_QUEUE_LEN) {
        q.pop();
    }
    q.push(word);
}

// sends a byte,address, and bank to the DAT bus
// value : 0-255
// address : 0-15
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
    for(int i = 0; i < 12; i++) {
        digitalWrite(datLines[i], word[i]);
    }
    // wait for the circuit to catch up
    delayMicroseconds(2);
    
    // latch the word with the bank signal
    digitalWrite(bank==0?DAT13:DAT14, 1);
    delayMicroseconds(3);
    digitalWrite(bank==0?DAT13:DAT14, 0);
    delayMicroseconds(3);
}

void unsetDATBusLevelShiftHiZ(void) {
    digitalWrite(DAT_BUS_LEVEL_SHIFTER_OE, 1);
}

void setDATBusLevelShifterHiZ(void) {
    digitalWrite(DAT_BUS_LEVEL_SHIFTER_OE, 0);
}

// writes a typical 20 word packet to the DAT bus when DAT0 falls
void ARDUINO_ISR_ATTR dat0isr() {
    for(int i = 0; i < 20; i++) {
        writeDAT(dspSettings[typicalPacketAddresses[i]][typicalPacketBanks[i]], typicalPacketAddresses[i], typicalPacketBanks[i]);
    }
}

// reads a word from the DAT bus when DAT13 rises
void ARDUINO_ISR_ATTR dat13isr() {
    busWord = 0;
    for (int i = 0; i < 12; i++) {
        busWord |= (digitalRead(datLines[i]) << i);
    }
    busWord |= (1<<12);
    // push to word stack with bank = 0
    tryPush(busWord, busQueue);
}

// reads a word from the DAT bus when DAT14 rises
void ARDUINO_ISR_ATTR dat14isr() {
    busWord = 0;
    for (int i = 0; i < 12; i++) {
        busWord |= (digitalRead(datLines[i]) << i);
    }
    busWord |= (1<<13);
    // push to word stack with bank = 1
    tryPush(busWord, busQueue);
}


// initialize the pins that will never change pinMode
void initFixedModePins(void) {
    pinMode(DAT0, INPUT);
    pinMode(DAT_BUS_LEVEL_SHIFTER_OE, OUTPUT);
    pinMode(BUF_CTL_LEVEL_SHIFTER_OE, OUTPUT);
    pinMode(NOT_BUF_OE, OUTPUT);
}

// writes some default values to DSP_Settings array
void initSettings(void) {
    // set all values to zero first
    for(int bank = 0; bank < 2; bank++) {
        for(int adr = 0; adr < 16; adr++) {
            dspSettings[adr][bank] = 0;
        }
    }

    int adr,bank,val;
    // load in default values for typical packet
    for(int i = 0; i < 20; i++) {
        adr = typicalPacketAddresses[i];
        bank = typicalPacketBanks[i];
        val = typicalPacketDefaultValues[i];
        dspSettings[adr][bank] = val;
    }
}

void setDAT1_14PinMode(int mode) {
    pinMode(DAT1, mode);
    pinMode(DAT2, mode);
    pinMode(DAT3, mode);
    pinMode(DAT4, mode);
    pinMode(DAT5, mode);
    pinMode(DAT6, mode);
    pinMode(DAT7, mode);
    pinMode(DAT8, mode);
    pinMode(DAT9, mode);
    pinMode(DAT10, mode);
    pinMode(DAT11, mode);
    pinMode(DAT12, mode);
    pinMode(DAT13, mode);
    pinMode(DAT14, mode);
}

void listenMode(void) {
    // disable interrupts
    noInterrupts();

    // set DAT BUS level shifters to Hi-Z
    digitalWrite(DAT_BUS_LEVEL_SHIFTER_OE, 0);

    // enable communication between native MCU and DSP
    // aka, enable the tristate buffers on the DAT Bus
    digitalWrite(NOT_BUF_OE, 0);

    // set all DAT pins to be inputs
    setDAT1_14PinMode(INPUT);

    // detatch interrupt from DAT0
    detachInterrupt(DAT0);

    // reconfigure interrupts to be triggered by DAT13 and DAT14
    attachInterrupt(DAT13, dat13isr, RISING);
    attachInterrupt(DAT14, dat14isr, RISING);

    // set DAT BUS level shifters to normal operation
    digitalWrite(DAT_BUS_LEVEL_SHIFTER_OE, 1);

    // enable interrupts
    interrupts();
}

void hijackMode(void) {
    // disable interrupts
    noInterrupts();

    // set DAT BUS level shifters to Hi-Z
    digitalWrite(DAT_BUS_LEVEL_SHIFTER_OE, 0);

    // disable communication between native MCU and DSP
    digitalWrite(NOT_BUF_OE, 1);

    // set all DAT pins to be outputs
    setDAT1_14PinMode(OUTPUT);

    // detatch interrupt from DAT13,DAT14
    detachInterrupt(DAT13);
    detachInterrupt(DAT14);

    // reconfigure interrupts to be triggered by DAT0
    attachInterrupt(DAT0, dat0isr, FALLING);

    // set DAT BUS level shifters to normal operation
    digitalWrite(DAT_BUS_LEVEL_SHIFTER_OE, 1);

    // enable interrupts
    interrupts();    
}

// configures all pins and sets the ESP into listen mode
void init(void) {
    initFixedModePins();
    // set BUF CTL level shifter to HiZ
    digitalWrite(BUF_CTL_LEVEL_SHIFTER_OE, 0);
     // set DAT BUS level shifters to Hi-Z
    digitalWrite(DAT_BUS_LEVEL_SHIFTER_OE, 0);
    // initialize DSP_Settings array
    initSettings();
    // initialize the queue
    // begin listen mode
    listenMode();
    // set BUF CTL level shifter to normal operation (and don't touch it again!)
    digitalWrite(BUF_CTL_LEVEL_SHIFTER_OE, 1);
}

