// iWRAP class demonstration Arduino sketch
// 8/31/2011 by Jeff Rowberg <jeff@rowberg.net>
// Updates should (hopefully) always be available at https://github.com/jrowberg

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
bool blinkState = false;
uint8_t ch;

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
}

void loop() {
    // passthrough from PC to iWRAP for debugging or manual control
    while (Serial.available()) Serial1.write(Serial.read());

    // check for module activity
    // (This method automatically watches for data from the module and parses
    // anything that comes in; you must either call this often or else manage
    // reading the data yourself and send it to the parse() method. This method
    // makes a non-blocking implementation easy though.)
    while (wt12.checkActivity()) { digitalWrite(LED_PIN, LOW); }
}

void onModuleIdle() {
    digitalWrite(LED_PIN, HIGH);
}

void onModuleBusy() {
    digitalWrite(LED_PIN, LOW);
}
