---
menu: Main
---
**A MIDI-controlled mono-voice digital synthesizer built on top of the AVR DB microcontroller series.**

![db-synth PCB](@@/p/db-synth/kicad/db-synth_20240111_top_1080.png)
db-synth PCB, revision 20240111

## Overview

db-synth is a digital synthesizer that runs entirely on an AVR DB series microcontroller, taking advantage of the integrated DAC and operational amplifiers available in this chip family. It implements a single synthesizer voice with a band-limited oscillator, an ADSR envelope generator, and a first-order digital filter -- all controllable via standard MIDI messages.

The design is bare-metal, with no operating system or HAL layer. The audio pipeline -- oscillator, envelope, amplifier, and filter -- runs in the main loop at a 48 kHz sample rate with 10-bit resolution, driven by the internal DAC and shaped through the on-chip opamps that implement a second-order reconstruction filter before reaching the audio output.

## Key highlights

- **Standard MIDI control** -- all synthesizer parameters are accessible via MIDI Control Change messages over a hardware MIDI interface with thru output
- **Band-limited oscillator** -- four waveforms (square, sine, triangle, saw) using pre-computed wavetables with band limiting to reduce aliasing
- **ADSR envelope generator** -- attack, decay, sustain, and release with both linear and exponential (AS3310-style) curves, ranging from 2 ms to 20 s
- **First-order digital filter** -- low-pass and high-pass modes with cutoff from 20 Hz to 20 kHz
- **On-chip signal path** -- internal 10-bit DAC through two integrated opamps (unity gain buffer and second-order reconstruction filter)
- **OLED parameter display** -- SSD1306-based display showing current synthesizer settings in real time over I2C
- **EEPROM preset storage** -- current settings can be saved to internal EEPROM and automatically restored on power-up
- **Open source** -- firmware licensed under BSD-3-Clause, hardware under CERN-OHL-S-2.0

## Explore further

- [Hardware](10_hardware.md) -- PCB design, connectors, enclosure, and display
- [Firmware](20_firmware.md) -- building, flashing, and architecture overview
- [MIDI implementation](30_midi.md) -- complete MIDI implementation chart
- [Source Code](https://github.com/rafaelmartins/db-synth) -- full project repository on GitHub
