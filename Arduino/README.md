## iWRAP Host Controller Library Arduino Implementation

| Board                        | Host Comms | Module Comms    | Reset | TX | RX |
|------------------------------|------------|-----------------|-------|----|----|
| Arduino Uno, Pro Mini, *328P | `Serial` (UART) | `AltSoftSerial` (SW UART) | 12 | 8 | 9 |
| Teensy 2.0                   | `Serial` (USB) | `Serial1` (HW UART) | 12 | 7 | 8 |
| Teensy++ 2.0                 | `Serial` (USB) | `Serial1` (HW UART) | 12 | 2 | 3 |

![Arduino iWRAP generic demo](../blob/master/Images/arduino_iwrap_demo_generic.png?raw=true)

![WT12 connection to Arduino Pro Mini 3.3v](../blob/master/Images/wt12_arduino_pro_mini.jpg?raw=true)

![WT12 connection to Teensy 2.0 w/3.3v mod](../blob/master/Images/wt12_teensy2.jpg?raw=true)
