
#include "Arduino.h"
#include <DATBus.h>

// DEBUG DATA
uint8_t data[] = {68, 149, 184, 103, 38, 174, 64, 130, 20, 255, 0, 0, 247, 0, 0, 60, 47, 147, 25, 14};
uint8_t addr[] = {9, 11, 3, 5, 1, 8, 7, 7, 0, 5, 11, 4, 13, 13, 0, 8, 12, 4, 2, 12};
uint8_t bank[] = {1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1};

void setup() {
    // initialize inputs and outputs for DAT bus
    setupPins();
    enableLevelShifterOutputs();
    enableMuxOutputs();
    setMuxChannel(MCU_CH); //set MCU to control the DSP


    // TODO: initialize BLE

}

void loop() {
    for(int i = 0; i<sizeof(data); i++) {
        writeDAT(data[i], addr[i], bank[i]);
    }
    delay(16);
}

