/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "midi.h"
#include "midi-data.h"


void
midi_init(midi_t *m, midi_channel_cb_t ch, midi_system_cb_t sys)
{
    if (m == NULL || m->_initialized)
        return;

    PORTC.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTC.PIN1CTRL = PORT_PULLUPEN_bm;
    USART1.BAUD = midi_usart_baud;
    USART1.CTRLC = midi_usart_cmode | USART_PMODE_DISABLED_gc | USART_SBMODE_1BIT_gc | USART_CHSIZE_8BIT_gc;
    PORTC.DIRSET = PIN0_bm;
    USART1.CTRLB = USART_TXEN_bm | USART_RXEN_bm | midi_usart_rxmode;

    m->_channel_cb = ch;
    m->_system_cb = sys;
    m->_initialized = true;
}


void
midi_task(midi_t *m)
{
    if (m == NULL || !m->_initialized)
        return;

    // run some pending callback and return
    if (m->_idx > 0 && m->_idx == m->_len + 1) {
        midi_command_t cmd = m->_buf[0] >> 4;
        switch (cmd) {
        case MIDI_SYSTEM:
            if (m->_system_cb != NULL)
                m->_system_cb(m->_buf[0] & 0xf, m->_buf + 1, m->_len);
            break;
        default:
            if (m->_channel_cb != NULL)
                m->_channel_cb(cmd, m->_buf[0] & 0xf, m->_buf + 1, m->_len);
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
        if (m->_idx)
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
