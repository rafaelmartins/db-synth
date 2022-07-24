/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>
#include "adsr.h"
#include "oled.h"
#include "oscillator.h"
#include "midi.h"
#include "midi-data.h"


void
midi_init(void)
{
    PORTC_PIN1CTRL = PORT_PULLUPEN_bm;
    USART1.BAUD = midi_usart_baud;
    USART1.CTRLC = midi_usart_cmode | USART_PMODE_DISABLED_gc | USART_SBMODE_1BIT_gc | USART_CHSIZE_8BIT_gc;
    USART1.CTRLB = USART_RXEN_bm | midi_usart_rxmode;
}


static uint8_t
midi_get_data_len(uint8_t b)
{
    switch (b >> 4) {
        case 0x0c:  // Program Change
        case 0x0d:  // Channel Pressure
            return 1;
        case 0x08:  // Note Off
        case 0x09:  // Note On
        case 0x0a:  // Polyphonic Pressure
        case 0x0b:  // Control Change
        case 0x0e:  // Pitch Bend
            return 2;
        case 0x0f:  // System
            switch (b & 0xf) {
                case 0x00:  // SOX (would be more than 0 if we support sysex)
                case 0x04:  // Undefined
                case 0x05:  // Undefined
                case 0x06:  // Tune Request
                case 0x07:  // EOX
                case 0x08:  // RT Timing Clock
                case 0x09:  // RT Undefined
                case 0x0a:  // RT Start
                case 0x0b:  // RT Continue
                case 0x0c:  // RT Stop
                case 0x0d:  // RT Undefined
                case 0x0e:  // RT Active Sense
                case 0x0f:  // RT System Reset
                    return 0;
                case 0x01:  // Time Code Quarter Frame
                case 0x03:  // Song Select
                    return 1;
                case 0x02:  // Song Position
                    return 2;
            }
            return 0;
    }
    return 0;
}


static void
midi_push_event(uint8_t *b, uint8_t len)
{
    if (len == 0) {
        return;
    }

    // if first byte is data, just ignore everything (probably sysex)
    if (!(b[0] & (1 << 7))) {
        return;
    }

    // ignore sysex (for now)
    static bool sysex = false;
    if (b[0] == 0xf0) {
        sysex = true;
        return;
    }
    else if (sysex) {
        sysex = false;
        if (b[len - 1] == 0xf7) {
            return;
        }
    }

    uint8_t cmd = b[0] >> 4;
    uint8_t chan = b[0] & 0xf;

    if (chan > 1)
        return;

    switch (cmd) {
    case 0x9:  // Note On
        if (len == 3 && b[1] != 0) {
            oscillator_set_note(b[1]);
            adsr_set_velocity(b[2]);
            adsr_set_gate();

            // FIXME: test code, results in some audio clicks
            // char buf[5];
            // buf[0] = '0';
            // buf[1] = 'x';
            // buf[2] = (b[1] >> 4) > 10 ? (b[1] >> 4) - 10 + 'a' : (b[1] >> 4) + '0';
            // buf[3] = (b[1] & 0xf) > 10 ? (b[1] & 0xf) - 10 + 'a' : (b[1] & 0xf) + '0';
            // buf[4] = 0;
            // oled_line(3, buf, OLED_HALIGN_CENTER);

            return;
        }

    // fall through
    case 0x8:  // Note Off
        if (len > 1) {
            adsr_unset_gate();
            return;
        }
    }
}


void
midi_task(void)
{
    if (!(USART1.STATUS & USART_RXCIF_bm))
        return;

    static uint8_t buf[3];
    static uint8_t idx = 0;
    static uint8_t len = 0;

    uint8_t b = USART1.RXDATAL;

    if (b & (1 << 7)) {  // status
        idx = 0;
        len = midi_get_data_len(b);
    }
    else if (idx > len) {
        return;
    }

    buf[idx] = b;

    if (idx++ == len) {
        midi_push_event(buf, len + 1);
    }
}
