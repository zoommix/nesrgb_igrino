# NESRGB_IGR.ino

A simple program that reads NES controller inputs and either resets the console or changes the NESRGB (clone with two palette pins) video palette. It’s heavily based on SukkoPera's [NESPlusPlus](https://github.com/SukkoPera/NESPlusPlus/tree/master) and JeffWDH's [NES-In-Game-Reset](https://github.com/JeffWDH/NES-In-Game-Reset), but with a different method of reading controller inputs, as the original didn’t work for me (possibly due to using a toploader NES console). This version is also much simpler.

The program is designed for use with NES toploader consoles modified with the NESRGB mod. The Arduino board used is an Arduino Nano clone.

## Connections

```
                                                        +-----+
|  NESRGB                       |          +------------| USB |------------+
| G | 1 | 2 |                   |          |            +-----+            |
+–––––––––––––––––––––––––––––––+          | [ ]D13/SCK        MISO/D12[ ] |
  |   |   |                                | [ ]3.3V           MOSI/D11[ ] |
+––––3v–––––+                              | [ ]V.ref            SS/D10[ ] |
| 5V to 3V  |             Controller CLOCK | [X]A0                   D9[ ] |
|  shifter  |             Controller LATCH | [X]A1                   D8[X] | Reset Out (Button)
+––––5v–––––+              Controller DATA | [X]A2                   D7[ ] |
  |       |                Reset Out (CPU) | [X]A3                   D6[ ] |
  |       +–––––NESRGB Pad 1 (via shifter) | [X]A4/SDA               D5[ ] |
  +–––––––––––––NESRGB Pad 2 (via shifter) | [X]A5/SCL               D4[ ] |
                                           | [ ]A6              INT1/D3[ ] |
                                           | [ ]A7              INT0/D2[ ] |
                            Controller +5V | [X]5V                  GND[X] | Controller GND
                                           | [ ]RST                 RST[ ] |
                                           | [ ]GND                 RX0[ ] |
                                           | [ ]Vin                 TX1[ ] |
                                           |                               |
                                           | NANO-V3                       |
                                           +-------------------------------+
```

## Controls

- Start + Select + A + B: Reset the console
- Start + Select + Right: Switch to the next palette
- Start + Select + Left: Switch to the previous palette
