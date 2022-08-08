/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <avr/io.h>
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

typedef struct {
    void (*channel_cb)(midi_command_t cmd, uint8_t ch, volatile uint8_t *buf, uint8_t len);
    void (*system_cb)(midi_system_subcommand_t cmd, volatile uint8_t *buf, uint8_t len);
    uint8_t _buf[3];
    uint8_t _idx;
    uint8_t _len;
} midi_t;


static inline void
midi_hw_init(void)
{
    PORTC.PIN1CTRL = PORT_PULLUPEN_bm;
    USART1.BAUD = midi_usart_baud;
    USART1.CTRLC = midi_usart_cmode | USART_PMODE_DISABLED_gc | USART_SBMODE_1BIT_gc | USART_CHSIZE_8BIT_gc;
    USART1.CTRLB = USART_RXEN_bm | midi_usart_rxmode;
}


void midi_task(volatile midi_t *m);
