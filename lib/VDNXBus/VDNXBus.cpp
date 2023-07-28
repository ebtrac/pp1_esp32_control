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
    transmitListenModeData = false;
    injectUserPacket = false;
}

uint8_t VDNXBus::bitreverse(uint8_t b, int len) {
    if(len == 2) {
        // return ((reversed>>6)&0x03);
        return b; // do not need to reverse the bank bits. 
    }
    uint8_t reversed = 0;
    for (int i = 0; i < 8; ++i) {
        reversed = (reversed << 1) | (b & 1);
        b >>= 1;
    }
    if(len == 4) {
        return ((reversed>>4)&0xF);
    }
    if(len == 8) {
        return reversed;
    }
    return 0;
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

bool VDNXBus::getWriteUserPacketFlag() {
    return writeUserPacketFlag;
}

void VDNXBus::removeWordFromUserPacket(uint8_t address, uint8_t bank) {
    uint8_t a,b;
    a = bitreverse(address,4);
    b = bitreverse(bank,2);
    userPacket.erase({a,b});
}

void VDNXBus::resetUserPacket()
{
    // clear the packet if it's not empty
    clearUserPacket();

    // fill the user packet
    PacketWord pakword;
    for(int i = 0; i < 20; i++) {
        pakword.address = bitreverse(typicalPacketAddresses[i],4);
        pakword.bank = bitreverse(typicalPacketBanks[i],2);
        pakword.value = bitreverse(typicalPacketDefaultValues[i],8);
        addWordToUserPacket(pakword);
    }
}

void VDNXBus::clearUserPacket() {
    if(!userPacket.empty()) {
        userPacket.clear();
    }
}

int VDNXBus::getUserPacket(uint16_t *words) {
    if(!userPacket.empty()) {
        for(const auto& item : userPacket) {
            *words++ = item.second;
        }
        return userPacket.size();
    }
    return 0;
}

void VDNXBus::setTransmitListenModeData(bool val) {
    transmitListenModeData = val;
}

bool VDNXBus::getTransmitListenModeData() {
    return transmitListenModeData;
}

void VDNXBus::setInjectUserPacket(bool val) {
    injectUserPacket = val;
}

bool VDNXBus::getInjectUserPacket() {
    return injectUserPacket;
}

void VDNXBus::addWordToUserPacket(uint16_t word) {
    PacketWord pakword;
    pakword.word = word;
    addWordToUserPacket(pakword);
}

void VDNXBus::addWordToUserPacket(PacketWord pakword) {
    userPacket[std::make_pair((uint8_t)pakword.address, (uint8_t)pakword.bank)] = pakword.word;
}

void VDNXBus::addWordToUserPacket(uint8_t value, uint8_t address, uint8_t bank) {
    PacketWord pakword;
    pakword.value = bitreverse(value,8);
    pakword.address = bitreverse(address,4);
    pakword.bank = bitreverse(bank,2);
    addWordToUserPacket(pakword);
}

void VDNXBus::removeWordFromUserPacket(uint16_t word) {
    PacketWord pakword;
    pakword.word = word;
    removeWordFromUserPacket(pakword);
}

void VDNXBus::removeWordFromUserPacket(PacketWord pakword) {
    userPacket.erase({(uint8_t)pakword.address, (uint8_t)pakword.bank});
}

void VDNXBus::writePacket(const UserPacketMap& packet) {
    // item represents a key,value pair
    // item.first is the key (the address and bank as a std::pair)
    // item.second is the value (a 16-bit word)
    for(const auto& item : packet) {
        // write the value and address b0..b11 out to the Bus
        for(uint8_t i = 0; i < 12; i++) {
            digitalWrite(datPins[i], (item.second >> i) & 0x1);
        }
        // wait for circuit to catch up (doesnt work inside an ISR...)
        delayMicroseconds(2);

        // latch the word with the bank selection (doesnt work inside an ISR...)
        digitalWrite(((item.second>>12)&0x1)==1?DAT13:DAT14, 1);
        delayMicroseconds(3);
        digitalWrite(((item.second>>12)&0x1)==1?DAT13:DAT14, 0);
        delayMicroseconds(3);
    }
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

// sets the flag that will allow attemptWriteUserPacketToBus() to fire once
void IRAM_ATTR VDNXBus::dat0isr() {
    // for(int i = 0; i < 20; i++) {
    //     writeDAT(dspSettings[typicalPacketAddresses[i]][typicalPacketBanks[i]], typicalPacketAddresses[i], typicalPacketBanks[i]);
    // }
    if(injectUserPacket) {
        // for(int i = 0; i < userPacket.length; i++) {
        //     writeDAT(userPacket.data[i].value, userPacket.data[i].address, userPacket.data[i].bank);
        // }
        writeUserPacketFlag = true;
    } else {
        writeUserPacketFlag = false;
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

void VDNXBus::attemptWriteUserPacketToBus() {
    if (!(writeUserPacketFlag && injectUserPacket))
        return;
    if (mode != Mode_Hijack)
        return;
    if(userPacket.empty())
        return;
    writePacket(userPacket);
    writeUserPacketFlag = false;
}

BusMode VDNXBus::getMode()
{
    return mode;
}

// initialize the pins that will never change pinMode
void VDNXBus::initFixedModePins(void) {
    pinMode(DAT0, INPUT);
    pinMode(VID_BLK, INPUT);
    pinMode(NOT_LISTEN, OUTPUT);
    pinMode(NOT_HIJACK, OUTPUT);
    pinMode(NOT_BUF_OE, OUTPUT);
}

// writes some default values to DSP_Settings array
void VDNXBus::initSettings(void) {
    // set all values to zero first
    for(uint8_t bank = 0; bank < 2; bank++) {
        for(uint8_t adr = 0; adr < 16; adr++) {
            VDNXMem[{bitreverse(adr,4), bitreverse(bank,2)}] = 0;
        }
    }

    uint8_t adr,bank,val;
    // load in default values for typical packet
    for(uint8_t i = 0; i < 20; i++) {
        adr = typicalPacketAddresses[i];
        bank = typicalPacketBanks[i];
        val = typicalPacketDefaultValues[i];
        VDNXMem[{bitreverse(adr,4), bitreverse(bank,2)}] = bitreverse(val,8);
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
    digitalWrite(NOT_HIJACK, 1);
    delayMicroseconds(50);

    // set all DAT pins to be inputs with pullups!
    setDAT1_14PinMode(INPUT_PULLUP); // ~15.5 us.

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
    digitalWrite(NOT_LISTEN, 0);

    // enable interrupts
    interrupts();

    mode = Mode_Listen;
}

void VDNXBus::hijackMode(void) {
    if (mode == Mode_Hijack) return;
    // disable interrupts
    if(mode != Mode_Init) noInterrupts();

    // set LISTEN level shifters to Hi-Z
    digitalWrite(NOT_LISTEN, 1);
    delayMicroseconds(50);

    // set all DAT pins to be outputs
    setDAT1_14PinMode(OUTPUT); // 15.7~34.4 us. DAT 1 took much longer than the rest.

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
    digitalWrite(NOT_HIJACK, 0);

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
    digitalWrite(NOT_HIJACK, 1);
     // set LISTEN level shifters to Hi-Z
    digitalWrite(NOT_LISTEN, 1);
    // initialize DSP_Settings array
    initSettings();
    resetUserPacket();
    // begin listen mode
    listenMode();
}