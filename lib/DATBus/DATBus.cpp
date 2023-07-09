#include <Arduino.h>
#include <DATBus.h>
#include <DATBusPins.h>
#include <SimpleCollections.h>
#include <SCCircularBuffer.h>

#define MAX_BUFFER_LEN 100

#define FAST_READ(pin_index) ((*datPinPorts[pin_index] & datPinMasks[pin_index])!=0)

uint8_t datPins[] = {DAT1, DAT2, DAT3, DAT4, DAT5, DAT6, DAT7, DAT8, DAT9, DAT10, DAT11, DAT12, DAT13, DAT14};
uint32_t datPinMasks[14];
volatile uint32_t *datPinPorts[14];

uint8_t dspSettings[16][2]; //first index is address, second index is bank

uint8_t typicalPacketAddresses[] = {9, 11, 3, 5, 1, 8, 7, 7, 0, 5, 11, 4, 13, 13, 0, 8, 12, 4, 2, 12};
uint8_t typicalPacketBanks[] = {1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1};
uint8_t typicalPacketDefaultValues[] = {68,149,184,103,38,174,64,130,20,255,0,0,247,0,0,60,47,147,25,14};

uint16_t busWord13 = 0; // temporarily stores the word being read from the bus during an isr
uint16_t busWord14 = 0; // temporarily stores the word being read from the bus during an isr

GenericCircularBuffer<uint16_t> busBuffer(MAX_BUFFER_LEN);

#define MODE_INIT 0
#define MODE_LISTEN 1
#define MODE_HIJACK 2
uint8_t mode = MODE_INIT; 

uint16_t getBuffer() {
    return busBuffer.get();
}

bool busBufferAvailable() {
    return busBuffer.available();
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
        digitalWrite(datPins[i], word[i]);
    }
    // wait for the circuit to catch up
    delayMicroseconds(2);
    
    // latch the word with the bank signal
    digitalWrite(bank==0?DAT13:DAT14, 1);
    delayMicroseconds(3);
    digitalWrite(bank==0?DAT13:DAT14, 0);
    delayMicroseconds(3);
}

// writes a typical 20 word packet to the DAT bus when DAT0 falls
void IRAM_ATTR dat0isr() {
    for(int i = 0; i < 20; i++) {
        writeDAT(dspSettings[typicalPacketAddresses[i]][typicalPacketBanks[i]], typicalPacketAddresses[i], typicalPacketBanks[i]);
    }
}

// reads a word from the DAT bus when DAT13 rises
void IRAM_ATTR dat13isr() {
    busWord13 = 0;
    // for (uint8_t i = 0; i < 12; i++) {
    //     // busWord13 |= (digitalRead(datPins[i]) << i);
    //     // busWord13 |= (digitalRead(datPins[i]) << i);
    //     busWord13 |= (FAST_READ(i) << i);
    // }
    busWord13 |= (FAST_READ(0) << 0);
    busWord13 |= (FAST_READ(1) << 1);
    busWord13 |= (FAST_READ(2) << 2);
    busWord13 |= (FAST_READ(3) << 3);
    busWord13 |= (FAST_READ(4) << 4);
    busWord13 |= (FAST_READ(5) << 5);
    busWord13 |= (FAST_READ(6) << 6);
    busWord13 |= (FAST_READ(7) << 7);
    busWord13 |= (FAST_READ(8) << 8);
    busWord13 |= (FAST_READ(9) << 9);
    busWord13 |= (FAST_READ(10) << 10);
    busWord13 |= (FAST_READ(11) << 11);
    busWord13 |= (1<<12);
    // push to word stack with bank = 0
    busBuffer.put(busWord13);
}

// reads a word from the DAT bus when DAT14 rises
void IRAM_ATTR dat14isr() {
    busWord14 = 0;
    // for (uint8_t i = 0; i < 12; i++) {
    //     // busWord14 |= (digitalRead(datPins[i]) << i);
    //     busWord14 |= (FAST_READ(i) << i);
    // }
    busWord14 |= (FAST_READ(0) << 0);
    busWord14 |= (FAST_READ(1) << 1);
    busWord14 |= (FAST_READ(2) << 2);
    busWord14 |= (FAST_READ(3) << 3);
    busWord14 |= (FAST_READ(4) << 4);
    busWord14 |= (FAST_READ(5) << 5);
    busWord14 |= (FAST_READ(6) << 6);
    busWord14 |= (FAST_READ(7) << 7);
    busWord14 |= (FAST_READ(8) << 8);
    busWord14 |= (FAST_READ(9) << 9);
    busWord14 |= (FAST_READ(10) << 10);
    busWord14 |= (FAST_READ(11) << 11);
    busWord14 |= (1<<13);
    // push to word stack with bank = 1
    busBuffer.put(busWord14);
}


// initialize the pins that will never change pinMode
void initFixedModePins(void) {
    pinMode(DAT0, INPUT);
    pinMode(LISTEN, OUTPUT);
    pinMode(HIJACK, OUTPUT);
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
    if (mode == MODE_LISTEN) return;

    // disable interrupts
    if(mode != MODE_INIT) noInterrupts();

    // set HIJACK level shifters to Hi-Z
    digitalWrite(HIJACK, 0);

    // enable communication between native MCU and DSP
    // aka, enable the tristate buffers on the DAT Bus
    digitalWrite(NOT_BUF_OE, 0);

    // set all DAT pins to be inputs with pullups!
    setDAT1_14PinMode(INPUT_PULLUP);

    // detatch interrupt from DAT0
    if(mode == MODE_HIJACK) detachInterrupt(digitalPinToInterrupt(DAT0));

    // reconfigure interrupts to be triggered by DAT13 and DAT14
    attachInterrupt(digitalPinToInterrupt(DAT13), dat13isr, RISING);
    attachInterrupt(digitalPinToInterrupt(DAT14), dat14isr, RISING);

    // set LISTEN level shifters to normal operation
    digitalWrite(LISTEN, 1);

    // enable interrupts
    interrupts();

    mode = MODE_LISTEN;
}

void hijackMode(void) {
    if (mode == MODE_HIJACK) return;
    // disable interrupts
    noInterrupts();

    // set LISTEN level shifters to Hi-Z
    digitalWrite(LISTEN, 0);

    // disable communication between native MCU and DSP
    digitalWrite(NOT_BUF_OE, 1);

    // set all DAT pins to be outputs
    setDAT1_14PinMode(OUTPUT);

    // detatch interrupt from DAT13,DAT14
    if(mode == MODE_LISTEN) {
        detachInterrupt(digitalPinToInterrupt(DAT13));
        detachInterrupt(digitalPinToInterrupt(DAT14));
    }

    // reconfigure interrupts to be triggered by DAT0
    attachInterrupt(digitalPinToInterrupt(DAT0), dat0isr, FALLING);

    // set HIJACK level shifters to normal operation
    digitalWrite(HIJACK, 1);

    // enable interrupts
    interrupts();

    mode = MODE_HIJACK;
}

void initPinMasksAndPorts() {
    for(int i = 0; i<14; i++) {
        datPinMasks[i] = digitalPinToBitMask(datPins[i]);
        datPinPorts[i] = portInputRegister(digitalPinToPort(datPins[i]));
    }
}

// configures all pins and sets the ESP into listen mode
void initDatBus(void) {
    mode = MODE_INIT;
    initPinMasksAndPorts();
    initFixedModePins();
    // set HIJACK level shifter to HiZ
    digitalWrite(HIJACK, 0);
     // set LISTEN level shifters to Hi-Z
    digitalWrite(LISTEN, 0);
    // initialize DSP_Settings array
    initSettings();
    // initialize the queue
    // begin listen mode
    listenMode();
}

