#include <Arduino.h>
#include <DATBus.h>
#include <BluetoothSerial.h>

#define BUF_SIZE 128
String device_name = "ESP32-VDNX-PP-1";

BluetoothSerial SerialBT;

char response[BUF_SIZE];
int responsepos = 0;
// DEBUG DATA
// uint8_t data[] = {68, 149, 184, 103, 38, 174, 64, 130, 20, 255, 0, 0, 247, 0, 0, 60, 47, 147, 25, 14};
// uint8_t addr[] = {9, 11, 3, 5, 1, 8, 7, 7, 0, 5, 11, 4, 13, 13, 0, 8, 12, 4, 2, 12};
// uint8_t bank[] = {1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1};

void setup() {
    initDatBus();

    Serial.begin(115200);

    SerialBT.begin(device_name);

}

void loop() {
  // if(SerialBT.available()) {
  //   // expects the following string format
  //   // 00_00_0\r\n
  //   // which is in hex
  //   // first byte : value
  //   // second byte : address
  //   // end : bank 
  //   SerialBT.read();
  //   SerialBT.readBytes(response + responsepos, 1);
  //   if (responsepos < BUF_SIZE) {
  //     responsepos++;
  //   } else {
  //     responsepos = 0;
  //   }
  // }

  if(SerialBT.connected() && busBufferAvailable()) {
    uint16_t bufItem = getBuffer();
    // write a bus queue item to the BLE in hex followed by /r/n
    SerialBT.printf("%04x\r\n", bufItem);
  }

  //DEBUG: write debug data
    // for(int i = 0; i<sizeof(data); i++) {
    //     writeDAT(data[i], addr[i], bank[i]);
    // }
    // delay(16);
}


