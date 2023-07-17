#include "Arduino.h"
#include <VDNXBusPins.h>
#include <SimpleCollections.h>
#include <SCCircularBuffer.h>
#include <map>
#include <utility>

#define MAX_BUFFER_LEN 256

enum BusMode {
    Mode_Init, Mode_Listen, Mode_Hijack
};

// struct PacketWord {
//     uint8_t value;
//     uint8_t address;
//     uint8_t bank;
// };

// used for storing the packet words in a compressed manner. ready to send.
// IMPORTANT: value and address must be big-endian 
union PacketWord {
    struct {
        uint8_t value   :8; //value bits    b0..b7
        uint8_t address :4; //address bits  b8..b11
        uint8_t bank    :2; //bank bits     b12..b13
        uint8_t         :2; //unused bits   b14..b15
    };
    uint16_t word; // combined representation of the PacketWord's elements
};

// struct UserPacket {
//     PacketWord data[32];
    
//     uint8_t length = 0;
// };

typedef std::map<std::pair<uint8_t, uint8_t>, uint8_t> AddressBankMap; // keys {address, bank} : values: uint8_t value
typedef std::map<std::pair<uint8_t, uint8_t>, uint16_t> UserPacketMap; // keys {address, bank} : values: uint16_t word

class VDNXBus {
    public:
        VDNXBus();

        uint8_t bitreverse(uint8_t b, int len); // quickly reverses the bits in a given byte

        void hijackMode();

        void listenMode();

        void initDatBus();

        uint16_t getBuffer();

        bool getDAT0();

        bool busBufferAvailable();

        bool getWriteUserPacketFlag();

        /// @brief writes the userPacket to the bus. does nothing if injectUserPacket or writeUserPacketFlag are false. does nothing if not in hijack mode
        void attemptWriteUserPacketToBus();

        BusMode getMode();

        void setTransmitListenModeData(bool val);
        bool getTransmitListenModeData();

        void setInjectUserPacket(bool val);
        bool getInjectUserPacket();

        /// @brief adds a word to the user packet.
        /// @param word word must be formatted big endian like PacketWord.
        void addWordToUserPacket(uint16_t word);
        void addWordToUserPacket(PacketWord pakword);

        /// @brief adds a word to the user packet. args are little endian, it will reverse them for you.
        /// @param value little endian, value to store
        /// @param address little endian, address to store the value at
        /// @param bank little endian, bank to store the value at
        void addWordToUserPacket(uint8_t value, uint8_t address, uint8_t bank);

        /// @brief removes a word from the user packet.
        /// @param word word must be formatted big endian like PacketWord.
        void removeWordFromUserPacket(uint16_t word);
        void removeWordFromUserPacket(PacketWord pakword);

        /// @brief removes a word from the user packet. args are little endian, it will reverse them for you.
        /// @param address little endian, address of the word
        /// @param bank little endian, bank of hte word
        void removeWordFromUserPacket(uint8_t address, uint8_t bank);

        void resetUserPacket();
        void clearUserPacket();
        int getUserPacket(uint16_t *words);
        int getUserPacketSize() {
            return userPacket.size();
        }

        void IRAM_ATTR dat0isr();
        void IRAM_ATTR dat13isr();
        void IRAM_ATTR dat14isr();

    private:
        BusMode mode;
        volatile bool transmitListenModeData; // controls whether data will be transmitted via Bluetooth in listen mode
        UserPacketMap userPacket;
        volatile bool injectUserPacket;

        uint8_t datPins[14] = {DAT1, DAT2, DAT3, DAT4, DAT5, DAT6, DAT7, DAT8, DAT9, DAT10, DAT11, DAT12, DAT13, DAT14};
        uint32_t datPinMasks[14];
        volatile uint32_t *datPinPorts[14];

        AddressBankMap VDNXMem; // key {address, bank}. emulates DSP's memory addresses. NOTE: big-endian
        

        uint8_t typicalPacketAddresses[20] =     { 9, 11,  3,  5, 1,  8, 7,  7, 0,  5,11, 4, 13, 13, 0, 8, 12,  4, 2, 12};
        uint8_t typicalPacketBanks[20] =         { 2,  2,  2,  2, 1,  2, 2,  1, 2,  1, 1, 2,  1,  2, 1, 1,  1,  1, 2,  2};
        uint8_t typicalPacketDefaultValues[20] = {68,149,184,103,38,174,64,130,20,255, 0, 0,247,  0, 0,60, 47,147,25, 14};

        uint16_t busWord13 = 0; // temporarily stores the word being read from the bus during an isr
        uint16_t busWord14 = 0; // temporarily stores the word being read from the bus during an isr

        volatile bool writeUserPacketFlag = false; // flag that becomes set when dat0isr fires. poll to know when to write the packet.

        GenericCircularBuffer<uint16_t> *busBuffer;

        unsigned long debouncing_time = 1; // debouncing time in microseconds
        volatile unsigned long last_microsDAT13 = 0; 
        volatile unsigned long last_microsDAT14 = 0; 

        static VDNXBus* instance;
        static IRAM_ATTR void dat0isrWrapper();
        static IRAM_ATTR void dat13isrWrapper();
        static IRAM_ATTR void dat14isrWrapper();

        bool IRAM_ATTR debounceDAT13();
        bool IRAM_ATTR debounceDAT14();

        void writePacket(const UserPacketMap& pak);

        void initFixedModePins();
        void initSettings();
        void initPinMasksAndPorts();
        void setDAT1_14PinMode(int pinmode);


};
