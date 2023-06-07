#include "Arduino.h"

#define MCU_CH 0
#define ESP_CH 1

void setupPins(void);
void writeDAT(uint8_t value, uint8_t address, uint8_t bank);

void enableLevelShifterOutputs(void);

void disableLevelShifterOutputs(void);

void disableMuxOutputs(void);

void enableMuxOutputs(void);

void setMuxChannel(int channel);

int getMuxChannel(void);
