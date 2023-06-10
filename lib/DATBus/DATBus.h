#include "Arduino.h"

void hijackMode(void);

void listenMode(void);

void initDatBus();

size_t getBusQueueSize();

uint16_t getBusQueueFront();

bool busQueueEmpty();

void popBusQueue();

void writeDAT(uint8_t value, uint8_t address, uint8_t bank);
