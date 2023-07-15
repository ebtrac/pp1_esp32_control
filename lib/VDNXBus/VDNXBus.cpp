#include <VDNXBus.h>
#include <VDNXBusPins.h>
#include <Arduino.h>
#include <SimpleCollections.h>
#include <SCCircularBuffer.h>

#define FAST_READ(pin_index) ((*datPinPorts[pin_index] & datPinMasks[pin_index])!=0)

GenericCircularBuffer<uint16_t> buf(MAX_BUFFER_LEN);

VDNXBus* VDNXBus::instance = nullptr;

VDNXBus::VDNXBus() {
    this->busBuffer = &buf;
    this->mode = Mode_Init;
    instance = this;
}

uint16_t VDNXBus::getBuffer() {
    return busBuffer->get();
}

bool VDNXBus::busBufferAvailable() {
    return busBuffer->available();
}

bool VDNXBus::getDAT0(void) {
    return FAST_READ(0);
}

// sends a byte,address, and bank to the DAT bus
// value : 0-255
// address : 0-15
// bank : 0-1 (0 = A = DAT13, 1 = B = DAT14)
void VDNXBus::writeDAT(uint8_t value, uint8_t address, uint8_t bank) {
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

void IRAM_ATTR VDNXBus::dat0isrWrapper() {
    if(instance != nullptr)
        instance->dat0isr();
}

void IRAM_ATTR VDNXBus::dat13isrWrapper() {
    if(instance != nullptr)
        instance->dat13isr();
}

void IRAM_ATTR VDNXBus::dat14isrWrapper() {
    if(instance != nullptr)
        instance->dat14isr();
}

// writes a typical 20 word packet to the DAT bus when DAT0 falls
void IRAM_ATTR VDNXBus::dat0isr() {
    for(int i = 0; i < 20; i++) {
        writeDAT(dspSettings[typicalPacketAddresses[i]][typicalPacketBanks[i]], typicalPacketAddresses[i], typicalPacketBanks[i]);
    }
}

//return true if pin was high for at least `debouncing_time` microseconds
bool IRAM_ATTR VDNXBus::debounceDAT13() {
    if (FAST_READ(12) == 1) { //dat 13 is high
        last_microsDAT13 = micros();
        return false;
    }
    // dat13 is low
    return ((micros() - last_microsDAT13) >= debouncing_time);
}

//return true if pin was high for at least `debouncing_time` microseconds
bool IRAM_ATTR VDNXBus::debounceDAT14() {
    if (FAST_READ(13) == 1) { //dat 14 is high
        last_microsDAT14 = micros(); //store the current time
        return false;
    }
    // dat14 is low
    return ((micros() - last_microsDAT14) >= debouncing_time); 
}

// reads a word from the DAT bus when DAT13 has been high for at least `debouncing_time` microseconds before falling
void IRAM_ATTR VDNXBus::dat13isr() {
    if(debounceDAT13()) {
        //busWord13 = 0;
        // for (uint8_t i = 0; i < 12; i++) {
        //     // busWord13 |= (digitalRead(datPins[i]) << i);
        //     // busWord13 |= (digitalRead(datPins[i]) << i);
        //     busWord13 |= (FAST_READ(i) << i);
        // }
        // busWord13 |= (FAST_READ(0) << 0);
        // busWord13 |= (FAST_READ(1) << 1);
        // busWord13 |= (FAST_READ(2) << 2);
        // busWord13 |= (FAST_READ(3) << 3);
        // busWord13 |= (FAST_READ(4) << 4);
        // busWord13 |= (FAST_READ(5) << 5);
        // busWord13 |= (FAST_READ(6) << 6);
        // busWord13 |= (FAST_READ(7) << 7);
        // busWord13 |= (FAST_READ(8) << 8);
        // busWord13 |= (FAST_READ(9) << 9);
        // busWord13 |= (FAST_READ(10) << 10);
        // busWord13 |= (FAST_READ(11) << 11);
        // busWord13 |= (1<<12);
        
        //busWord13 = GPIO.in;


        busWord13 = (FAST_READ(0) << 0)|(FAST_READ(1) << 1)|(FAST_READ(2) << 2)|(FAST_READ(3) << 3)| (FAST_READ(4) << 4)|(FAST_READ(5) << 5)|(FAST_READ(6) << 6)
        |(FAST_READ(7) << 7)|(FAST_READ(8) << 8)|(FAST_READ(9) << 9)|(FAST_READ(10) << 10)|(FAST_READ(11) << 11)|(1<<12);
        // push to word stack with bank = 0
        busBuffer->put(busWord13);
    }
}

// reads a word from the DAT bus when DAT14 has been high for at least `debouncing_time` microseconds before falling
void IRAM_ATTR VDNXBus::dat14isr() {
    if(debounceDAT14()) {
       // busWord14 = 0;
        // for (uint8_t i = 0; i < 12; i++) {
        //     // busWord14 |= (digitalRead(datPins[i]) << i);
        //     busWord14 |= (FAST_READ(i) << i);
        // }
        // busWord14 |= (FAST_READ(0) << 0);
        // busWord14 |= (FAST_READ(1) << 1);
        // busWord14 |= (FAST_READ(2) << 2);
        // busWord14 |= (FAST_READ(3) << 3);
        // busWord14 |= (FAST_READ(4) << 4);
        // busWord14 |= (FAST_READ(5) << 5);
        // busWord14 |= (FAST_READ(6) << 6);
        // busWord14 |= (FAST_READ(7) << 7);
        // busWord14 |= (FAST_READ(8) << 8);
        // busWord14 |= (FAST_READ(9) << 9);
        // busWord14 |= (FAST_READ(10) << 10);
        // busWord14 |= (FAST_READ(11) << 11);
        // busWord14 |= (1<<13);

        busWord14 = (FAST_READ(0) << 0)|(FAST_READ(1) << 1)|(FAST_READ(2) << 2)|(FAST_READ(3) << 3)| (FAST_READ(4) << 4)|(FAST_READ(5) << 5)|(FAST_READ(6) << 6)
        |(FAST_READ(7) << 7)|(FAST_READ(8) << 8)|(FAST_READ(9) << 9)|(FAST_READ(10) << 10)|(FAST_READ(11) << 11)|(1<<13);
        // push to word stack with bank = 1
        busBuffer->put(busWord14);
    }
}

BusMode VDNXBus::getMode() {
    return mode;
}

// initialize the pins that will never change pinMode
void VDNXBus::initFixedModePins(void) {
    pinMode(DAT0, INPUT);
    pinMode(LISTEN, OUTPUT);
    pinMode(HIJACK, OUTPUT);
    pinMode(NOT_BUF_OE, OUTPUT);
}

// writes some default values to DSP_Settings array
void VDNXBus::initSettings(void) {
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

void VDNXBus::setDAT1_14PinMode(int pinmode) {
    pinMode(DAT1, pinmode);
    pinMode(DAT2, pinmode);
    pinMode(DAT3, pinmode);
    pinMode(DAT4, pinmode);
    pinMode(DAT5, pinmode);
    pinMode(DAT6, pinmode);
    pinMode(DAT7, pinmode);
    pinMode(DAT8, pinmode);
    pinMode(DAT9, pinmode);
    pinMode(DAT10, pinmode);
    pinMode(DAT11, pinmode);
    pinMode(DAT12, pinmode);
    pinMode(DAT13, pinmode);
    pinMode(DAT14, pinmode);
}

void VDNXBus::listenMode(void) {
    if (mode == Mode_Listen) return;

    // disable interrupts
    if(mode != Mode_Init) noInterrupts();

    // set HIJACK level shifters to Hi-Z
    digitalWrite(HIJACK, 0);
    delayMicroseconds(50);

    // set all DAT pins to be inputs with pullups!
    setDAT1_14PinMode(INPUT_PULLUP);

    // enable communication between native MCU and DSP
    // aka, enable the tristate buffers on the DAT Bus
    digitalWrite(NOT_BUF_OE, 0);
    delayMicroseconds(50);

    // detatch interrupt from DAT0
    if(mode == Mode_Hijack) detachInterrupt(digitalPinToInterrupt(DAT0));

    // reconfigure interrupts to be triggered by DAT13 and DAT14
    attachInterrupt(digitalPinToInterrupt(DAT13), VDNXBus::dat13isrWrapper, CHANGE);
    attachInterrupt(digitalPinToInterrupt(DAT14), VDNXBus::dat14isrWrapper, CHANGE);

    // set LISTEN level shifters to normal operation
    digitalWrite(LISTEN, 1);

    // enable interrupts
    interrupts();

    mode = Mode_Listen;
}

void VDNXBus::hijackMode(void) {
    if (mode == Mode_Hijack) return;
    // disable interrupts
    if(mode != Mode_Init) noInterrupts();

    // set LISTEN level shifters to Hi-Z
    digitalWrite(LISTEN, 0);
    delayMicroseconds(50);

    // set all DAT pins to be outputs
    setDAT1_14PinMode(OUTPUT);

    // disable communication between native MCU and DSP
    digitalWrite(NOT_BUF_OE, 1);
    delayMicroseconds(50);

    // detatch interrupt from DAT13,DAT14
    if(mode == Mode_Listen) {
        detachInterrupt(digitalPinToInterrupt(DAT13));
        detachInterrupt(digitalPinToInterrupt(DAT14));
    }

    // reconfigure interrupts to be triggered by DAT0
    attachInterrupt(digitalPinToInterrupt(DAT0), VDNXBus::dat0isrWrapper, FALLING);

    // set HIJACK level shifters to normal operation
    digitalWrite(HIJACK, 1);

    // enable interrupts
    interrupts();

    mode = Mode_Hijack;
}

void VDNXBus::initPinMasksAndPorts() {
    for(int i = 0; i<14; i++) {
        datPinMasks[i] = digitalPinToBitMask(datPins[i]);
        datPinPorts[i] = portInputRegister(digitalPinToPort(datPins[i]));
    }
}

// configures all pins and sets the ESP into listen mode
void VDNXBus::initDatBus(void) {
    mode = Mode_Init;
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