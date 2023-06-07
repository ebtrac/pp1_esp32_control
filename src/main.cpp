
#include "Arduino.h"
#include <DATBus.h>

// DEBUG DATA
uint8_t data[] = {68, 149, 184, 103, 38, 174, 64, 130, 20, 255, 0, 0, 247, 0, 0, 60, 47, 147, 25, 14};
uint8_t addr[] = {9, 11, 3, 5, 1, 8, 7, 7, 0, 5, 11, 4, 13, 13, 0, 8, 12, 4, 2, 12};
uint8_t bank[] = {1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1};

void setup() {
    // initialize inputs and outputs for DAT bus
    setupPins();

    // TODO: initialize I2C

    // TODO: initialize WiFi/BLE

}

void loop() {
    for(int i = 0; i<sizeof(data); i++) {
        writeDAT(data[i], addr[i], bank[i]);
    }
    delay(16);
}


/*
 WiFi Web Server LED Blink

 A simple web server that lets you blink an LED via the web.
 This sketch will print the IP address of your WiFi Shield (once connected)
 to the Serial monitor. From there, you can open that address in a web browser
 to turn on and off the LED on pin 5.

 If the IP address of your shield is yourAddress:
 http://yourAddress/H turns the LED on
 http://yourAddress/L turns it off

 This example is written for a network using WPA2 encryption. For insecure
 WEP or WPA, change the Wifi.begin() call and use Wifi.setMinSecurity() accordingly.

 Circuit:
 * WiFi shield attached
 * LED attached to pin 5

 created for arduino 25 Nov 2012
 by Tom Igoe

ported for sparkfun esp32 
31.01.2017 by Jan Hendrik Berlin
 
 */

/* #include <WiFi.h>

const char* ssid     = "NBBGL";
const char* password = "ijustdrinkblooddude666";

WiFiServer server(80);

void setup()
{
    Serial.begin(115200);
    pinMode(2, OUTPUT);      // set the LED pin mode

    delay(10);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    server.begin();

}

void loop(){
 WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/H\">here</a> to turn the LED on pin 2 on.<br>");
            client.print("Click <a href=\"/L\">here</a> to turn the LED on pin 2 off.<br>");
            client.print("<form>");
            client.print("<label for=\"fname\">First name:</label><br>");
            client.print("<input type=\"text\" id=\"fname\" name=\"fname\"><br>");
            client.print("<label for=\"lname\">Last name:</label><br>");
            client.print("<input type=\"text\" id=\"lname\" name=\"lname\"><br><br>");
            client.print("<input type=\"submit\" value=\"Submit\">");
            client.print("</form>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(2, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(2, LOW);                // GET /L turns the LED off
        }

        if (currentLine.indexOf("?") >= 0) {

        }

      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
} */