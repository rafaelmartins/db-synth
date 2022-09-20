/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "midi-data.h"

typedef enum {
    MIDI_NOTE_OFF = 0x8,
    MIDI_NOTE_ON,
    MIDI_POLYPHONIC_PRESSURE,
    MIDI_CONTROL_CHANGE,
    MIDI_PROGRAM_CHANGE,
    MIDI_CHANNEL_PRESSURE,
    MIDI_PITCH_BEND,
    MIDI_SYSTEM,
} midi_command_t;

typedef enum {
    MIDI_SYSTEM_SYSEX1,
    MIDI_SYSTEM_TIME_CODE_QUARTER_FRAME,
    MIDI_SYSTEM_SONG_POSITION,
    MIDI_SYSTEM_SONG_SELECT,
    MIDI_SYSTEM_UNDEFINED1,
    MIDI_SYSTEM_UNDEFINED2,
    MIDI_SYSTEM_TUNE_REQUEST,
    MIDI_SYSTEM_SYSEX2,
    MIDI_SYSTEM_RT_TIMING_CLOCK,
    MIDI_SYSTEM_RT_UNDEFINED1,
    MIDI_SYSTEM_RT_START,
    MIDI_SYSTEM_RT_CONTINUE,
    MIDI_SYSTEM_RT_STOP,
    MIDI_SYSTEM_RT_UNDEFINED2,
    MIDI_SYSTEM_RT_ACTIVE_SENSE,
    MIDI_SYSTEM_RT_SYSTEM_RESET,
} midi_system_subcommand_t;

typedef void (*midi_channel_cb_t)(midi_command_t cmd, uint8_t ch, uint8_t *buf, uint8_t len);
typedef void (*midi_system_cb_t)(midi_system_subcommand_t cmd, uint8_t *buf, uint8_t len);

typedef struct {
    bool _initialized;
    uint8_t _buf[3];
    uint8_t _idx;
    uint8_t _len;
    midi_channel_cb_t _channel_cb;
    midi_system_cb_t _system_cb;
} midi_t;

void midi_init(midi_t *m, midi_channel_cb_t ch, midi_system_cb_t sys);
void midi_task(midi_t *m);
