# db-synth

A MIDI-controlled mono-voice digital synthesizer built on top of the AVR DB microcontroller series.

> **Quick access**
>
> - ["I want to build a db-synth!"](#hardware)
> - ["I want to flash the firmware to a db-synth!"](#software)
> - ["I want to setup my MIDI controller for db-synth!"](#midi-implementation-chart)


## Why?!

I've been playing with the design of synthesizers (both analog and digital) for some time, and I'm a long time AVR user. When I noticed that Microchip released a new [AVR DB series](https://www.microchip.com/en-us/products/microcontrollers-and-microprocessors/8-bit-mcus/avr-mcus/avr-db), that included internal DACs and operational amplifiers, building a digital synthesizer out of it was the only obvious thing to do! :D


## Features

This synthesizer implements a single but reasonably capable voice, with the following modules:

- Band-limited oscillator (tracks most of the MIDI note range, with some instabilities around 8th/9th octaves)
  + Square (*not* PWM)
  + Sine
  + Triangle
  + Saw
- ADSR envelope generator (time parameters from 20ms to 20s, emulating `AS3310`)
  + Linear curves
  + Exponential curves (emulating `AS3310`)
- First order (1 pole) filter (cutoff range from 20Hz to 20kHz)
  + Low pass
  + High pass

Other features include:

- 48kHz/10bit sample rate.
- Fully controllable via MIDI.
- Current settings can be stored into the internal EEPROM as a preset.
- Second order reconstruction filter for signal output.
- OLED display for parameter monitoring.


## Hardware

### Printed Circuit Board

A PCB for this synthesizer was designed using [Kicad](https://www.kicad.org/):

- [Schematics](./pcb/db-synth.pdf)
<!-- - [Interactive Bill of Materials](https://rafaelmartins.github.io/db-synth/ibom/)-->

A 3D-printable enclosure was designed using [OpenSCAD](https://openscad.org/):

- [Front](./3d-models/enclosure-front.stl) ([source](./3d-models/enclosure-front.scad))
- [Back](./3d-models/enclosure-back.stl) ([source](./3d-models/enclosure-back.scad))


## Software

### Download prebuilt binaries

TODO

### ... or compile from source

To build the firmware, you need an AVR toolchain with support for the AVR DB series. Linux users can download it from https://github.com/rafaelmartins/avr-toolchain/releases/latest. Just extract it somewhere and add the `avr/bin` folder to your `PATH` variable. [`cmake`](https://cmake.org/) is also required.

```console
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ../firmware
$ make
```

This will create the `db-synth.hex` and `db-synth-fuse.hex` files.

### Write to microcontroller

We can write to the microcontroller memory spaces using the UPDI interface. I recommend using the [`pymcuprog`](https://github.com/microchip-pic-avr-tools/pymcuprog) tool, from Microchip itself, with a [Serial port UPDI adapter](https://github.com/microchip-pic-avr-tools/pymcuprog#serial-port-updi-pyupdi), that can be easily built using a TTL serial adapter and a resistor, as described in the link. This was NOT tested with the AVR128DB48 Curiosity Nano or similar evaluation boards.

With the adapter connected to the UPDI port in the db-synth PCB, and configured as described in the [`pymcuprog`](https://github.com/microchip-pic-avr-tools/pymcuprog) documentation, run (assuming that serial adapter is available at `/dev/ttyUSB0` and that it supports a baudrate of `230400`):

```console
$ pymcuprog -t uart -d avr128db28 --uart /dev/ttyUSB0 -c 230400 write -f db-synth-fuse.hex  --erase
$ pymcuprog -t uart -d avr128db28 --uart /dev/ttyUSB0 -c 230400 write -f db-synth.hex  --erase
```

The first command will write fuses, and the second command will write the firmware to flash.

#### Troubleshooting

If the `pymcuprog` command fails on Linux with some permission problem, make sure to add your user to the group that owns the device `/dev/ttyUSB0` (in our example). You can check which group is it, by running:

```console
$ ls -l /dev/ttyUSB0
crw-rw---- 1 root dialout 188, 3 Aug 29 00:06 /dev/ttyUSB0
```

In this example, the group is `dialout`. Just add yourself to the group:

```console
$ sudo gpasswd -a $USER dialout
```


### MIDI Implementation Chart

<table>
    <thead>
        <tr>
            <th colspan="2">Function</th>
            <th>Transmitted</th>
            <th>Recognized</th>
            <th>Remarks</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td rowspan="2">Basic Channel</td>
            <td>Default</td>
            <td>×</td>
            <td>1 - 16</td>
            <td>Memorized</td>
        </tr>
        <tr>
            <td>Changed</td>
            <td>×</td>
            <td>1 - 16</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td rowspan="3">Mode</td>
            <td>Default</td>
            <td>×</td>
            <td>4</td>
            <td>Mode 4: Omni Off, Mono</td>
        </tr>
        <tr>
            <td>Messages</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>Altered</td>
            <td>-</td>
            <td>-</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td rowspan="2">Note Number</td>
            <td>&nbsp;</td>
            <td>×</td>
            <td>0 - 127</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>True Voice</td>
            <td>-</td>
            <td>0 - 117</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td rowspan="2">Velocity</td>
            <td>Note On</td>
            <td>×</td>
            <td>○</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>Note Off</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td rowspan="2">After Touch</td>
            <td>Key's</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>Channel's</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td colspan="2">Pitch Bend</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td rowspan="10">Control Change</td>
            <td>3</td>
            <td>×</td>
            <td>○</td>
            <td>
                Oscillator Waveform Type<br />
                <ul>
                    <li>0 - 31 (Square)</li>
                    <li>32 - 63 (Sine)</li>
                    <li>64 - 95 (Triangle)</li>
                    <li>95 - 127 (Saw)</li>
                </ul>
            </td>
        </tr>
        <tr>
            <td>70</td>
            <td>×</td>
            <td>○</td>
            <td>
                ADSR Envelope Type
                <ul>
                    <li>0 - 63 (Exponential)</li>
                    <li>64 - 127 (Linear)</li>
                </ul>
            </td>
        </tr>
        <tr>
            <td>71</td>
            <td>×</td>
            <td>○</td>
            <td>
                Filter Type
                <ul>
                    <li>0 - 42 (Off)</li>
                    <li>43 - 85 (Low pass)</li>
                    <li>86 - 127(High pass)</li>
                </ul>
            </td>
        </tr>
        <tr>
            <td>72</td>
            <td>×</td>
            <td>○</td>
            <td>ADSR Envelope Release (20ms - 20s)</td>
        </tr>
        <tr>
            <td>73</td>
            <td>×</td>
            <td>○</td>
            <td>ADSR Envelope Attack (20ms - 20s)</td>
        </tr>
        <tr>
            <td>74</td>
            <td>×</td>
            <td>○</td>
            <td>Filter Cutoff (20Hz - 20kHz)</td>
        </tr>
        <tr>
            <td>75</td>
            <td>×</td>
            <td>○</td>
            <td>ADSR Envelope Decay (20ms - 20s)</td>
        </tr>
        <tr>
            <td>79</td>
            <td>×</td>
            <td>○</td>
            <td>ADSR Envelope Sustain (0 - 100%)</td>
        </tr>
        <tr>
            <td>102</td>
            <td>×</td>
            <td>○</td>
            <td>
                Set MIDI Channel
                <ul>
                    <li>0 - 63 (No action)</li>
                    <li>64 - 127 (Set active channel to current channel)</li>
                </ul>
            </td>
        </tr>
        <tr>
            <td>119</td>
            <td>×</td>
            <td>○</td>
            <td>
                Write Settings to EEPROM
                <ul>
                    <li>0 - 63 (No action)</li>
                    <li>64 - 127 (Write settings)</li>
                </ul>
            </td>
        </tr>
        <tr>
            <td colspan="2">Program Change</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td colspan="2">System Exclusive</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td rowspan="3">System Common</td>
            <td>Song Position</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>Song Select</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>Tune Request</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td rowspan="2">System Real Time</td>
            <td>Clock</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>Commands</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td rowspan="6">Aux Messages</td>
            <td>All Sound Off</td>
            <td>×</td>
            <td>○</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>Reset All Controllers</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>Local On/Off</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>All Notes Off</td>
            <td>×</td>
            <td>○</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>Active Sensing</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>System Reset</td>
            <td>×</td>
            <td>×</td>
            <td>&nbsp;</td>
        </tr>
    </tbody>
</table>
