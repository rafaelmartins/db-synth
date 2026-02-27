# Firmware

The db-synth firmware is a bare-metal C application targeting the AVR DB series microcontrollers. It implements the complete audio synthesis pipeline -- oscillator, ADSR envelope, amplifier, and filter -- running in a polled main loop at a 48 kHz sample rate. All DSP data (wavetables, envelope curves, filter coefficients) is pre-computed by the [synth-datagen](@@/p/synth-datagen) tool and stored in program memory.

## Building from source

### Prerequisites

- [AVR GNU Toolchain](https://github.com/rafaelmartins/avr-toolchain/releases/latest) with AVR DB series support
- [CMake](https://cmake.org/) 3.25 or later
- [Ninja](https://ninja-build.org/) (recommended) or Make

The AVR toolchain must include `avr-gcc`, `avr-objcopy`, and `avr-size`. Download the toolchain, extract it, and add its `avr/bin` directory to `PATH`.

### Configure and build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build
```

To build for the AVR64DB28 instead of the default AVR128DB28:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DWITH_MCU=avr64db28 -G Ninja
cmake --build build
```

### Output artifacts

| File | Description |
|------|-------------|
| `db-synth.elf` | ELF binary with debug symbols |
| `db-synth.hex` | Intel HEX firmware image |
| `db-synth-fuse.hex` | Intel HEX fuse configuration |
| `db-synth.elf.map` | Linker map file |

The firmware version is derived from Git tags automatically.

## Flashing

Automated builds from the latest source are available from the [`rolling` release](https://github.com/rafaelmartins/db-synth/releases/tag/rolling).

### Using UPDI

The firmware is flashed to the microcontroller via the UPDI interface using the J1 header on the PCB. Two files must be written:

1. `db-synth-fuse.hex` -- fuse configuration (written first)
2. `db-synth.hex` -- firmware image (written second, with erase)

Use the appropriate `--device` value for the installed MCU: `avr128db28` or `avr64db28`.

For detailed UPDI flashing instructions using `pymcuprog`, refer to the [Hardware Build Manual](@@/hardware/build-manual/).

## Architecture overview

### Clock configuration

The internal high-frequency oscillator runs at 24 MHz with auto-tuning enabled. A single timer (TCB0) generates the 48 kHz sample rate by counting 500 clock cycles per period (24 MHz / 500 = 48 kHz). The main loop polls the TCB0 capture flag rather than using interrupts.

### Peripheral map

| Peripheral | Function |
|------------|----------|
| CLKCTRL | Internal oscillator at 24 MHz with auto-tuning |
| TCB0 | 48 kHz sample rate timer (CCMP = 500) |
| DAC0 | 10-bit audio DAC, VREF = 2.5V |
| OPAMP0 | Unity gain buffer for DAC output |
| OPAMP1 | 2nd-order low-pass reconstruction filter |
| USART1 (PC0/PC1) | MIDI TX/RX at 31250 baud |
| TWI0 (PA2/PA3) | I2C for SSD1306 OLED at 400 kHz |
| EEPROM | Preset / settings storage |

### Main loop

The main loop polls the TCB0 capture flag at 48 kHz without using interrupts. Each iteration runs the following tasks in order:

1. **MIDI task** -- reads and parses one incoming MIDI byte, retransmits it for thru
2. **Screen task** -- updates one OLED display line per iteration via the non-blocking I2C state machine
3. **Settings task** -- writes one pending EEPROM byte if a settings save is in progress
4. **Audio sample computation** -- computes and outputs a single audio sample through the signal path

The signal path computes each sample as follows:

1. **Oscillator** -- produces a signed 16-bit sample from band-limited wavetables using a phase accumulator. Waveform and note changes are synchronized to zero crossings to avoid clicks.
2. **Amplifier** -- scales the oscillator output by the ADSR envelope level and MIDI velocity using optimized AVR multiply instructions.
3. **Filter** -- applies a first-order IIR filter (low-pass or high-pass) to the amplified sample, also implemented with inline assembly for the fixed-point coefficient math.
4. **DAC output** -- the resulting sample is offset to unsigned range, clamped, and written to the 10-bit DAC.

The DAC output feeds OPAMP0 configured as a unity gain buffer, which feeds OPAMP1 configured as a second-order low-pass reconstruction filter before reaching the audio output connector.

### MIDI interface

MIDI input arrives on USART1 (PC1) at 31250 baud with optocoupler isolation (6N137). The parser handles running status and dispatches channel messages through a callback. Received bytes are retransmitted on the TX pin (PC0) to implement MIDI thru.

See the [MIDI implementation](30_midi.md) page for the complete implementation chart.

### OLED display

An SSD1306-based OLED display is driven over I2C (TWI0 on PA2/PA3) at 400 kHz. The display driver uses a non-blocking, state-machine-based rendering pipeline that updates one display line per main loop iteration. The screen shows the current waveform, MIDI channel, ADSR parameters, and filter settings. A notification system temporarily overlays messages (e.g., "PRESET UPDATED") for 2 seconds before reverting.

### Settings storage

Synthesizer parameters are stored in the AVR's internal EEPROM. On first boot, factory defaults are written. Settings writes are triggered via MIDI (CC 119) and processed incrementally -- one EEPROM byte per main loop iteration -- to avoid blocking the audio pipeline.

### Source files

| File | Purpose |
|------|---------|
| `main.c` | Initialization, main loop, MIDI message dispatch, fuse configuration |
| `oscillator.c` | Band-limited wavetable oscillator with phase accumulator |
| `adsr.c` | ADSR envelope generator with linear and AS3310-style exponential curves |
| `amplifier.c` | Sample amplitude scaling using AVR multiply instructions |
| `filter.c` | First-order IIR digital filter with fixed-point coefficient math |
| `midi.c` | MIDI message parser with running status and thru output |
| `oled.c` | SSD1306 OLED driver with non-blocking I2C rendering |
| `screen.c` | Display layout, parameter formatting, notification system |
| `settings.c` | EEPROM-backed settings storage with incremental writes |

### Generated data

Pre-computed DSP data is generated by [synth-datagen](@@/p/synth-datagen) and stored as C header files. Interactive charts of this data are available:

- [Oscillator waveform data](@@/p/db-synth/charts/oscillator-data.html) -- band-limited wavetables for each waveform across octaves
- [ADSR envelope data](@@/p/db-synth/charts/adsr-data.html) -- AS3310-style and linear envelope curves, time step tables
- [Filter coefficient data](@@/p/db-synth/charts/filter-data.html) -- low-pass and high-pass one-pole filter coefficients

| File | Contents |
|------|----------|
| `main-data.h` | Clock frequency, timer period, DAC offset constants |
| `midi-data.h` | USART baud rate register value |
| `oled-data.h` | TWI baud rate register value |
| `oscillator-data.h` | Wavetables (sine, band-limited square/triangle/saw), phase step tables, octave mapping |
| `adsr-data.h` | AS3310 and linear envelope curves, time step tables, parameter descriptions |
| `filter-data.h` | Low-pass and high-pass one-pole filter coefficients |
| `screen-data.h` | ADSR and filter parameter description strings |
