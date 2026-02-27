# MIDI implementation

db-synth is a MIDI receiver. It does not transmit any MIDI messages. The MIDI input includes hardware thru, where received bytes are retransmitted to the MIDI OUT/THRU connector, allowing multiple devices to be chained.

## Implementation chart

| Function | | Transmitted | Recognized | Remarks |
|---|---|---|---|---|
| Basic Channel | Default | x | 1--16 | Memorized |
| | Changed | x | 1--16 | |
| Mode | Default | x | 4 | Omni Off, Mono |
| | Messages | x | x | |
| | Altered | -- | -- | |
| Note Number | | x | 0--127 | |
| | True Voice | -- | 0--127 | |
| Velocity | Note On | x | o | |
| | Note Off | x | x | |
| After Touch | Key's | x | x | |
| | Channel's | x | x | |
| Pitch Bend | | x | x | |

## Control change

| CC | Function | Transmitted | Recognized | Values |
|---|---|---|---|---|
| 3 | Oscillator waveform | x | o | 0--31: Square, 32--63: Sine, 64--95: Triangle, 96--127: Saw |
| 70 | ADSR envelope type | x | o | 0--63: Exponential (AS3310-style), 64--127: Linear |
| 71 | Filter type | x | o | 0--41: Off, 42--83: Low pass, 84--127: High pass |
| 72 | ADSR release time | x | o | 2 ms -- 20 s |
| 73 | ADSR attack time | x | o | 2 ms -- 20 s |
| 74 | Filter cutoff frequency | x | o | 20 Hz -- 20 kHz |
| 75 | ADSR decay time | x | o | 2 ms -- 20 s |
| 79 | ADSR sustain level | x | o | 0--100% |
| 102 | Set MIDI channel | x | o | 0--63: No action, 64--127: Set to current message channel |
| 119 | Write settings to EEPROM | x | o | 0--63: No action, 64--127: Write current settings |
| 120 | All Sound Off | x | o | |
| 123 | All Notes Off | x | o | |

> [!NOTE]
> CC 102 (Set MIDI channel) is the only message processed regardless of the currently configured channel. All other messages are filtered by the active channel.

## Other messages

| Function | | Transmitted | Recognized | Remarks |
|---|---|---|---|---|
| Program Change | | x | x | |
| System Exclusive | | x | x | |
| System Common | Song Position | x | x | |
| | Song Select | x | x | |
| | Tune Request | x | x | |
| System Real Time | Clock | x | x | |
| | Commands | x | x | |
| Aux Messages | All Sound Off | x | o | CC 120 |
| | Reset All Controllers | x | x | |
| | Local On/Off | x | x | |
| | All Notes Off | x | o | CC 123 |
| | Active Sensing | x | x | |
| | System Reset | x | x | |

## Legend

| Symbol | Meaning |
|--------|---------|
| o | Recognized |
| x | Not recognized / Not transmitted |
| -- | Not applicable |
