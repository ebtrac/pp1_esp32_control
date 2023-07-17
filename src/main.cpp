#include <Arduino.h>
#include <VDNXBus.h>
#include <BluetoothSerial.h>
#include <string>

#define RX_BUF_SIZE 15

BluetoothSerial SerialBT;
VDNXBus Bus;
uint16_t bufItem;
char rxbuf[RX_BUF_SIZE];

void setup() {
  String device_name = "ESP32-VDNX-PP-1";
    Bus.initDatBus();
    SerialBT.begin(device_name);
}

void loop() {
  if (SerialBT.connected()) {
    // scan for user input
    if (SerialBT.available()) {
      // process user input
      SerialBT.readBytes(rxbuf, RX_BUF_SIZE);

      std::string cmd(rxbuf, rxbuf+2); // read the first 2 characters to get a command
      if (cmd == "mh") {
        Bus.hijackMode();
      }
      else if(cmd == "ml") {
        Bus.listenMode();
      }
      else if(cmd == "gm") { //get mode. 0=Init, 1=Listen, 2=Hijack
        SerialBT.println(Bus.getMode());
      }
      else if(cmd == "st") { //set transmit setting. example `st 1` enables transmit on listen mode
        Bus.setTransmitListenModeData(rxbuf[3] == '1');
      }
      else if(cmd == "gt") { //get transmit setting
        SerialBT.println((int)Bus.getTransmitListenModeData());
      }
      else if(cmd == "pr") { //packet reset. reset packet to default.
        Bus.resetUserPacket();
      } 
      else if(cmd == "pc") { //packet clear. clears the user packet entirely.
        Bus.clearUserPacket();
      }
      else if(cmd == "pa") { //packet add. adds a packet to the userpacket. raw bytes only
        uint16_t word = (rxbuf[3]<<8)|(rxbuf[4]);
        Bus.addWordToUserPacket(word);
      }
      else if(cmd == "pd") { //packet delete. deletes the packet at the given bank and address
        uint16_t word = (rxbuf[3]<<8)|(rxbuf[4]);
        Bus.removeWordFromUserPacket(word);
      }
      else if(cmd == "pg") { //packet get. prints the entire user packet. returns raw bytes separated by '\r\n'
        uint16_t words[32];
        int n = Bus.getUserPacket(words);
        for(int i = 0; i < n; i++) {
          SerialBT.write((words[i]>>8)&0xFF);
          SerialBT.write(words[i]&0xFF);
          SerialBT.println("");
        }
      }
      else if(cmd == "si") { //set inject setting. example 'si 1' enables packet injection in listen mode
        Bus.setInjectUserPacket(rxbuf[3] == '1');
      }
      else if(cmd == "gi") { //get inject setting.
        SerialBT.println((int)Bus.getInjectUserPacket());
      }
    }
  }

    // handle current mode operations
    switch(Bus.getMode()) {
      case Mode_Listen:
        if(SerialBT.connected()) {
          if(Bus.getTransmitListenModeData() && Bus.getDAT0() && Bus.busBufferAvailable()) {
            bufItem = Bus.getBuffer();
            // write a bus queue item to the BLE in hex followed by /r/n
            SerialBT.printf("%04x\r\n", bufItem); /////////////////////////////// How long does this take?
          }
        }
        break;

      case Mode_Hijack:
        Bus.attemptWriteUserPacketToBus();
        break;
      case Mode_Init:
        break;
    }
}



