#include "Arduino.h"

void hijackMode(void);

void listenMode(void);

void initDatBus();

uint16_t getBuffer();

bool getDAT0(void);

bool busBufferAvailable();

void writeDAT(uint8_t value, uint8_t address, uint8_t bank);
