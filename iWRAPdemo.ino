// Bluegiga iWRAP interface library demonstration Arduino sketch
// 9/11/2011 by Jeff Rowberg <jeff@rowberg.net>
// Updates should (hopefully) always be available at https://github.com/jrowberg/iwrap

/* ============================================
iWRAP library code is placed under the MIT license
Copyright (c) 2011 Jeff Rowberg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
===============================================
*/

// NOTE: This demo sketch assumes the use of an Arduino Mega with multiple
// hardware serial ports. The default "Serial" object is used to communicate
// with the host PC connected to the board via USB, while the "Serial1" object
// is used to communicate with a Bluegiga WT12 iWRAP4 module. The WT12 needs
// only 4 connections:
//
// WT12 GND -> Arduino GND
// WT12 VDD -> Arduino 3.3v (make SURE this is 3.3v, and not 5V)
// WT12 RXD -> Arduino TX1 (pin 18)
// WT12 TXD -> Arduino RX1 (pin 19)

#include "iWRAP.h"
iWRAP wt12(&Serial1, &Serial);

#define LED_PIN 13
bool linkActive = false;
bool blinkState = false;
uint16_t tick = 0;
uint8_t ch = 0;

// variables for fun mouse cursor circle tracking
int8_t x = -16, y = 0;
int8_t xDir = 1, yDir = -1;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // initialize serial communication with host
    Serial.begin(115200);
    
    // initialize iWRAP device
    Serial.println("Initializing iWRAP Bluetooth device serial connection...");
    Serial1.begin(115200);

    // assign special busy/idle callbacks because...well...because we can
    wt12.onBusy(onModuleBusy);
    wt12.onIdle(onModuleIdle);
    wt12.onRing(onModuleRing);
    wt12.onNoCarrier(onModuleNoCarrier);

    // make sure we end any incomplete lines in the WT12 buffer
    Serial1.print("\r\n");

    // reset iWRAP
    Serial.println("Rebooting iWRAP device...");
    wt12.setEchoModuleOutput(true);
    wt12.resetDevice();
    while (wt12.checkActivity());

    // get all configuration parameters and wait for completion
    Serial.println("Reading iWRAP configuration...");
    wt12.readDeviceConfig();
    while (wt12.checkActivity());

    // enable HID profile if necessary
    if (!wt12.config.profileHIDEnabled) {
        Serial.println("Enabling HID profile...");
        wt12.setProfile("HID", "iWRAP Demo Device");
        while (wt12.checkActivity());
        if (wt12.checkError()) {
            Serial.println("Uh oh, that didn't work out so well.");
        } else {
            Serial.println("Rebooting iWRAP device again to activate profile...");
            wt12.resetDevice();
            while (wt12.checkActivity());
            Serial.println("Re-reading iWRAP configuration...");
            wt12.readDeviceConfig();
            while (wt12.checkActivity());
        }
    }

    // output some info to prove we read the config correctly
    Serial.print("+ BT address=");
    Serial.print(wt12.config.btAddress.address[0], HEX); Serial.print(" ");
    Serial.print(wt12.config.btAddress.address[1], HEX); Serial.print(" ");
    Serial.print(wt12.config.btAddress.address[2], HEX); Serial.print(" ");
    Serial.print(wt12.config.btAddress.address[3], HEX); Serial.print(" ");
    Serial.print(wt12.config.btAddress.address[4], HEX); Serial.print(" ");
    Serial.println(wt12.config.btAddress.address[5], HEX);
    Serial.print("+ BT name=");
    Serial.println(wt12.config.btName);
    Serial.print("+ BT IDENT description=");
    Serial.println(wt12.config.btIdentDescription);
    Serial.print("+ UART baud rate=");
    Serial.println(wt12.config.uartBaudRate);
    Serial.print("+ Current pairing count=");
    Serial.println(wt12.config.btPairCount);
}

void loop() {
    // passthrough from PC to iWRAP for debugging or manual control
    while (Serial.available()) Serial1.write(Serial.read());

    // check for module activity
    // (This method automatically watches for data from the module and parses
    // anything that comes in; you must either call this often or else manage
    // reading the data yourself and send it to the parse() method. This method
    // makes a non-blocking implementation easy though.)
    while (wt12.checkActivity());
    
    tick++;
    if (++tick % 8192 == 0 && linkActive) { // tick wraps at 65535 automatically
        digitalWrite(LED_PIN, blinkState ? HIGH : LOW);
        blinkState = !blinkState;
    }
    if (tick % 2048 == 0 && linkActive) {
        // send data bytes to move the mouse cursor
        // (continuous small circles, a.k.a. "drunk mouse")
        Serial1.write(0x9f);
        Serial1.write(0x05);
        Serial1.write(0xa1);
        Serial1.write(0x02);
        Serial1.write((uint8_t)0x00);
        Serial1.write((int8_t)x);
        Serial1.write((int8_t)y);

        // poor man's ugly sine wave generator
        if (xDir == -1) x--; else x++;
        if (yDir == -1) y--; else y++;
        if (x == -16 || x == 16) xDir = -xDir;
        if (y == -16 || y == 16) yDir = -yDir;
    }
}

void onModuleIdle() {
    // turn indicator LED on
    digitalWrite(LED_PIN, HIGH);
}

void onModuleBusy() {
    // turn indicator LED off
    digitalWrite(LED_PIN, LOW);
}

void onModuleRing() {
    // enable blinking indicator
    delay(1000); // delay to let link "settle" (research?)
    linkActive = true;
}

void onModuleNoCarrier() {
    // disable blinking indicator
    linkActive = false;
}