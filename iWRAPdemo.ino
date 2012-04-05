// Bluegiga iWRAP interface library demonstration Arduino sketch
// 9/11/2011 by Jeff Rowberg <jeff@rowberg.net>
// Updates should (hopefully) always be available at https://github.com/jrowberg/iwrap

/* ============================================
iWRAP library code is placed under the MIT license
Copyright (c) 2012 Jeff Rowberg

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
// is used to communicate with a Bluegiga WT11/WT12 iWRAP4 module. The WT12
// needs a total of 6 connections for most efficient event-driven management:
//
// BT GND -> Arduino GND
// BT VDD -> Arduino 3.3v (make SURE this is 3.3v, and not 5v)
// BT RXD -> Arduino TX1 (pin 18)
// BT TXD -> Arduino RX1 (pin 19)
// BT PIO6 -> Arduino INT6
// BT PIO7 -> Arduino INT7
//
// If you are using the Teensy++ 2.0 board instead, the pins are as follows:
//
// BT GND -> Teensy++ GND
// BT VDD -> Teensy++ VCC (make SURE the Teensy++ is converted to 3.3v!)
// BT RXD -> Teensy++ TX1 (pin 18)
// BT TXD -> Teensy++ RX1 (pin 19)
// BT PIO6 -> Teensy++ INT6 (PE6)
// BT PIO7 -> Teensy++ INT7 (PE7)

#include "iWRAP.h"

void onModuleSelectLink(iWRAPLink *link);

#define ARDUINO_MEGA    10
#define TEENSYPP2       20

//#define BOARD ARDUINO_MEGA
#define BOARD TEENSYPP2

#if BOARD == ARDUINO_MEGA

    iWRAP bluetooth(&Serial1, &Serial);
    #define LED_PIN 13
    #define DTR_PIN 18
    #define LINK_PIN 19
    #define LINK_INT 7

#elif BOARD == TEENSYPP2

    HardwareSerial Serial1 = HardwareSerial();
    iWRAP bluetooth(&Serial1, (HardwareSerial *)&Serial);
    #define LED_PIN 6
    #define DTR_PIN 18
    #define LINK_PIN 19
    #define LINK_INT 7

#else

    #error Unknown board selected. iWRAP demo cannot run like this.

#endif


// variables for fun mouse cursor circle tracking
int8_t x = -16, y = 0;
int8_t xDir = 1, yDir = -1;

// variables for detecting mode and link status
bool linkActive = false;
bool dataMode = false;

// variables and interrupt routine for 10ms timer (simple LED blinking control)
volatile uint8_t timer1Overflow = 0;
uint8_t tick = 0;
ISR(TIMER1_OVF_vect) {
    timer1Overflow++;
}

void setup() {
    // initialize pins
    pinMode(LED_PIN, OUTPUT);
    pinMode(DTR_PIN, OUTPUT);
    pinMode(LINK_PIN, INPUT);
    digitalWrite(DTR_PIN, LOW);
    digitalWrite(LED_PIN, LOW);

    // setup internal 100Hz "tick" interrupt
    // thanks to http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1212098919 (and 'bens')
    // also, lots of timer info here and here:
    //    http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=50106
    //    http://www.avrbeginners.net/architecture/timers/timers.html
    TCCR1A = 1; // set TIMER1 overflow at 8-bit max (0xFF)
    TCCR1B = 1; // no prescaler
    TCNT1 = 0; // clear TIMER1 counter
    TIFR1 |= (1 << TOV1); // clear the TIMER1 overflow flag
    TIMSK1 |= (1 << TOIE1); // enable TIMER1 overflow interrupts

    // initialize serial communication with host
    Serial.begin(38400);

    // initialize iWRAP device
    // NOTE: default speed for this WT11/WT12 is 115200. This is not usually possible
    // unless you are running your microcontroller at 16MHz or more. 38400 works at
    // pretty much any speed though.
    Serial.println("Initializing iWRAP Bluetooth device serial connection...");
    Serial1.begin(38400);

    // assign manual control of entering/exiting data mode (uses DTR_PIN)
    bluetooth.onSelectLink(onModuleSelectLink);
    bluetooth.onExitDataMode(onModuleExitDataMode);

    // watch for READY event to further track command mode status
    bluetooth.onReady(onModuleReady);

    // enable local module -> PC passthrough echo for fun/debugging
    bluetooth.setEchoModuleOutput(true);
    
    // check for already active link
    if (digitalRead(LINK_PIN)) {
        linkActive = true;
        Serial.println("Device appears to have active data connection!");
        bluetooth.setDeviceMode(IWRAP_MODE_DATA);
    }

    // get all configuration parameters and wait for completion
    Serial.println("Reading iWRAP configuration...");
    bluetooth.readDeviceConfig();
    while (bluetooth.checkActivity(1000));
    if (bluetooth.checkError()) {
        Serial.println("iWRAP config read generated a syntax error, trying again...");
        bluetooth.readDeviceConfig();
        while (bluetooth.checkActivity(1000));
    }
    if (bluetooth.checkError() || bluetooth.checkTimeout()) {
        Serial.println("iWRAP config could not be read. Baud rate may be incorrect, or iWRAP stuck in DATA mode?");
    } else {
        // get list of active links
        Serial.println("Reading list link configuration...");
        bluetooth.readLinkConfig();
        while (bluetooth.checkActivity(1000));
        
        // enable HID profile if necessary
        if (!bluetooth.config.profileHIDEnabled) {
            Serial.println("Enabling HID profile...");
            bluetooth.setProfile("HID", "iWRAP Demo Device");
            while (bluetooth.checkActivity(1000));
            if (bluetooth.checkError()) {
                Serial.println("Uh oh, that didn't work out so well.");
            } else {
                Serial.println("Rebooting iWRAP device again to activate profile...");
                bluetooth.resetDevice();
                while (bluetooth.checkActivity());
                Serial.println("Re-reading iWRAP configuration...");
                bluetooth.readDeviceConfig();
                while (bluetooth.checkActivity(1000));
            }
        } else {
            Serial.println("Detected HID profile already enabled");
        }

        // set device name for demo
        Serial.println("Setting device name...");
        bluetooth.setDeviceName("Arduino iWRAPdemo");
        while (bluetooth.checkActivity(1000));

        // set device identity for demo
        Serial.println("Setting device identity...");
        bluetooth.setDeviceIdentity("Arduino iWRAPdemo Sketch");
        while (bluetooth.checkActivity(1000));

        // set device class to KB/mouse
        Serial.println("Setting device class to keyboard/mouse...");
        bluetooth.setDeviceClass(0x05C0);
        while (bluetooth.checkActivity(1000));

        // set SSP mode to 3 0
        Serial.println("Setting Secure Simple Pairing (SSP) mode to 3/0...");
        bluetooth.setDeviceSSP(3, 0);
        while (bluetooth.checkActivity(1000));

        // enable CD pair notification on GPIO pin 7
        Serial.println("Enabling GPIO7 toggling on Carrier Detect (link active mode)...");
        bluetooth.setCarrierDetect(0x80, 0);
        while (bluetooth.checkActivity(1000));

        // enable GPIO pin 7 interrupt detection
        Serial.println("Attaching interrupt to Carrier Detect pin change (GPIO7)...");
        attachInterrupt(LINK_INT, onLinkChange, CHANGE);

        // enable DTR escape control on GPIO pin 6
        Serial.println("Enabling GPIO6 DTR detection for entering command mode (ESCAPE char disabled)...");
        bluetooth.setControlEscape('-', 0x40, 1);
        while (bluetooth.checkActivity(1000));

        // output some info to prove we read the config correctly
        Serial.print("+ BT address=");
        Serial.print(bluetooth.config.btAddress.address[0], HEX); Serial.print(" ");
        Serial.print(bluetooth.config.btAddress.address[1], HEX); Serial.print(" ");
        Serial.print(bluetooth.config.btAddress.address[2], HEX); Serial.print(" ");
        Serial.print(bluetooth.config.btAddress.address[3], HEX); Serial.print(" ");
        Serial.print(bluetooth.config.btAddress.address[4], HEX); Serial.print(" ");
        Serial.println(bluetooth.config.btAddress.address[5], HEX);
        Serial.print("+ BT name=");
        Serial.println(bluetooth.config.btName);
        Serial.print("+ BT IDENT description=");
        Serial.println(bluetooth.config.btIdentDescription);
        Serial.print("+ UART baud rate=");
        Serial.println(bluetooth.config.uartBaudRate);
        Serial.print("+ Current pairing count=");
        Serial.println(bluetooth.config.btPairCount);

        if (digitalRead(LINK_PIN)) {
            Serial.println("Switching back to data mode for previously active link...");
            bluetooth.selectDataMode((uint8_t)0); // use default link since we don't know what it was
            while (bluetooth.checkActivity(1000));
        }
    }
}

void loop() {
    // passthrough from host to iWRAP for debugging or manual control
    while (Serial.available()) Serial1.write(Serial.read());

    // check for module activity
    // (This method automatically watches for data from the module and parses
    // anything that comes in; you must either call this often or else manage
    // reading the data yourself and send it to the parse() method. This method
    // makes a non-blocking implementation easy though.)
    while (bluetooth.checkActivity());

    // check for TIMER1 overflow limit and increment tick (should be every 10 ms)
    // 156 results in 9.937 ms, 157 results in 10.001 ms
    if (timer1Overflow >= 157) {
        timer1Overflow = 0;
        if (++tick >= 100) tick = 0;
        if (linkActive) {
            if (dataMode) {
                digitalWrite(LED_PIN, HIGH);
            } else {
                digitalWrite(LED_PIN, (tick % 25) < 13 ? HIGH : LOW);
            }
        } else {
            digitalWrite(LED_PIN, tick < 50 ? HIGH : LOW);
        }

        /*
        if (tick == 10) {
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
        */
    }
}

void onModuleExitDataMode() {
    Serial.println("Exiting data mode...");
    digitalWrite(DTR_PIN, LOW);
    delayMicroseconds(50);
    digitalWrite(DTR_PIN, HIGH);
    bluetooth.setDeviceMode(IWRAP_MODE_COMMAND);
    while (bluetooth.checkActivity(200));
    dataMode = false;
}
void onModuleSelectLink(iWRAPLink *link) {
    Serial.print("Selecting link ");
    Serial.println(link -> link_id);
    dataMode = true;
}

void onModuleReady() {
    // entered COMMAND mode, e.g. after a reset or DTR toggle
    Serial.println("iWRAP is in command mode, ready for control");
    dataMode = false;
}

void onLinkChange() {
    if (digitalRead(LINK_PIN) == HIGH) {
        // changed from LOW to HIGH
        //Serial.println("Active link detected, entering data mode...");
        linkActive = true;
        //dataMode = true;
    } else {
        // changed from HIGH to LOW
        Serial.println("All links closed, exiting data mode...");
        linkActive = false;
        dataMode = false;
    }
}