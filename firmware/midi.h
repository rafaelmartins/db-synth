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
#include <stdlib.h>
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
    void (*channel_cb)(midi_command_t cmd, uint8_t ch, uint8_t *buf, uint8_t len);
    void (*system_cb)(midi_system_subcommand_t cmd, uint8_t *buf, uint8_t len);
    uint8_t _buf[3];
    uint8_t _idx;
    uint8_t _len;
} midi_t;


static inline void
midi_hw_init(void)
{
    PORTC.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTC.PIN1CTRL = PORT_PULLUPEN_bm;
    USART1.BAUD = midi_usart_baud;
    USART1.CTRLC = midi_usart_cmode | USART_PMODE_DISABLED_gc | USART_SBMODE_1BIT_gc | USART_CHSIZE_8BIT_gc;
    PORTC.DIRSET = PIN0_bm;
    USART1.CTRLB = USART_TXEN_bm | USART_RXEN_bm | midi_usart_rxmode;
}


static inline void
midi_task(midi_t *m)
{
    if (m == NULL)
        return;

    // run some pending callback and return
    if (m->_idx > 0 && m->_idx == m->_len + 1) {
        midi_command_t cmd = m->_buf[0] >> 4;
        switch (cmd) {
        case MIDI_SYSTEM:
            if (m->system_cb != NULL)
                m->system_cb(m->_buf[0] & 0xf, m->_buf + 1, m->_len);
            break;
        default:
            if (m->channel_cb != NULL)
                m->channel_cb(cmd, m->_buf[0] & 0xf, m->_buf + 1, m->_len);
            break;
        }
        m->_idx = 0;
        m->_len = 0;
        return;
    }

    // nothing to read from usart, return
    if (!(USART1.STATUS & USART_RXCIF_bm))
        return;

    uint8_t b = USART1.RXDATAL;
    if (USART1.STATUS & USART_DREIF_bm)
        USART1.TXDATAL = b;

    if (b < 0x80) {  // data
        if (m->_idx == 0)  // no status yet
            return;

        m->_buf[m->_idx++] = b;
        return;
    }

    // status
    m->_idx = 0;
    m->_buf[m->_idx++] = b;

    switch ((midi_command_t) (b >> 4)) {
    case MIDI_PROGRAM_CHANGE:
    case MIDI_CHANNEL_PRESSURE:
        m->_len = 1;
        break;
    case MIDI_NOTE_OFF:
    case MIDI_NOTE_ON:
    case MIDI_POLYPHONIC_PRESSURE:
    case MIDI_CONTROL_CHANGE:
    case MIDI_PITCH_BEND:
        m->_len = 2;
        break;
    case MIDI_SYSTEM:
        switch ((midi_system_subcommand_t) (b & 0xf)) {
            case MIDI_SYSTEM_SYSEX1:
            case MIDI_SYSTEM_UNDEFINED1:
            case MIDI_SYSTEM_UNDEFINED2:
            case MIDI_SYSTEM_TUNE_REQUEST:
            case MIDI_SYSTEM_SYSEX2:
            case MIDI_SYSTEM_RT_TIMING_CLOCK:
            case MIDI_SYSTEM_RT_UNDEFINED1:
            case MIDI_SYSTEM_RT_START:
            case MIDI_SYSTEM_RT_CONTINUE:
            case MIDI_SYSTEM_RT_STOP:
            case MIDI_SYSTEM_RT_UNDEFINED2:
            case MIDI_SYSTEM_RT_ACTIVE_SENSE:
            case MIDI_SYSTEM_RT_SYSTEM_RESET:
                m->_len = 0;
                break;
            case MIDI_SYSTEM_TIME_CODE_QUARTER_FRAME:
            case MIDI_SYSTEM_SONG_SELECT:
                m->_len = 1;
                break;
            case MIDI_SYSTEM_SONG_POSITION:
                m->_len = 2;
                break;
        }
        break;
    }
}
