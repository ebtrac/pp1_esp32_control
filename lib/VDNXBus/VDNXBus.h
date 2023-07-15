#include "Arduino.h"
#include <VDNXBusPins.h>
#include <SimpleCollections.h>
#include <SCCircularBuffer.h>

#define MAX_BUFFER_LEN 256

enum BusMode {
    Mode_Init, Mode_Listen, Mode_Hijack
};

class VDNXBus {
    public:
        VDNXBus();

        void hijackMode();

        void listenMode();

        void initDatBus();

        uint16_t getBuffer();

        bool getDAT0();

        bool busBufferAvailable();

        void writeDAT(uint8_t value, uint8_t address, uint8_t bank);

        BusMode getMode();

        void IRAM_ATTR dat0isr();
        void IRAM_ATTR dat13isr();
        void IRAM_ATTR dat14isr();

    private:
        uint8_t datPins[14] = {DAT1, DAT2, DAT3, DAT4, DAT5, DAT6, DAT7, DAT8, DAT9, DAT10, DAT11, DAT12, DAT13, DAT14};
        uint32_t datPinMasks[14];
        volatile uint32_t *datPinPorts[14];

        uint8_t dspSettings[16][2]; //first index is address, second index is bank

        uint8_t typicalPacketAddresses[20] = {9, 11, 3, 5, 1, 8, 7, 7, 0, 5, 11, 4, 13, 13, 0, 8, 12, 4, 2, 12};
        uint8_t typicalPacketBanks[20] = {1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1};
        uint8_t typicalPacketDefaultValues[20] = {68,149,184,103,38,174,64,130,20,255,0,0,247,0,0,60,47,147,25,14};

        uint16_t busWord13 = 0; // temporarily stores the word being read from the bus during an isr
        uint16_t busWord14 = 0; // temporarily stores the word being read from the bus during an isr

        GenericCircularBuffer<uint16_t> *busBuffer;

        unsigned long debouncing_time = 1; // debouncing time in microseconds
        volatile unsigned long last_microsDAT13 = 0; 
        volatile unsigned long last_microsDAT14 = 0; 

        static VDNXBus* instance;
        static void dat0isrWrapper();
        static void dat13isrWrapper();
        static void dat14isrWrapper();

        BusMode mode;
        
        bool IRAM_ATTR debounceDAT13();
        bool IRAM_ATTR debounceDAT14();

        void initFixedModePins();
        void initSettings();
        void initPinMasksAndPorts();
        void setDAT1_14PinMode(int pinmode);

};
