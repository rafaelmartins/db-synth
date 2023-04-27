/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <avr/pgmspace.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "oscillator.h"
#include "oscillator-data.h"


void
oscillator_init(oscillator_t *o)
{
    if (o == NULL || o->_initialized)
        return;

    o->_initialized = true;
    o->_phase.data = 0;
    o->_waveform = OSCILLATOR_WAVEFORM__LAST;
    o->_waveform_next = OSCILLATOR_WAVEFORM__LAST;
    o->_note = 0xff;
    o->_note_next = 0xff;
}


bool
oscillator_set_waveform(oscillator_t *o, oscillator_waveform_t wf)
{
    if (o != NULL && o->_initialized && o->_waveform != wf) {
        o->_waveform_next = wf;
        return true;
    }
    return false;
}


void
oscillator_set_note(oscillator_t *o, uint8_t n)
{
    if (o != NULL && o->_initialized && n < notes_phase_steps_len)
        o->_note_next = n;
}


static bool
phase_step(oscillator_phase_t *p, uint8_t idx)
{
    if (p == NULL)
        return false;

    p->data += pgm_read_dword(&(notes_phase_steps[idx]));
    if (p->pint >= oscillator_sine_len) {
        p->pint -= oscillator_sine_len;
        return true;
    }
    return false;
}


int16_t
oscillator_get_sample(oscillator_t *o)
{
    if (o == NULL || !o->_initialized)
        return 0;

    if (o->_note >= notes_phase_steps_len) {  // not running
        if (o->_note_next >= notes_phase_steps_len || o->_waveform_next >= OSCILLATOR_WAVEFORM__LAST)  // no note to play yet
            return 0;
        o->_note = o->_note_next;
        o->_note_next = 0xff;
        o->_waveform = o->_waveform_next;
        o->_waveform_next = OSCILLATOR_WAVEFORM__LAST;
        o->_phase.data = 0;
    }
    else if (phase_step(&o->_phase, o->_note)) {
        if (o->_note_next < notes_phase_steps_len) {  // new note to play
            o->_note = o->_note_next;
            o->_note_next = 0xff;
        }
        if (o->_waveform_next < OSCILLATOR_WAVEFORM__LAST) {  // new waveform to set
            o->_waveform = o->_waveform_next;
            o->_waveform_next = OSCILLATOR_WAVEFORM__LAST;
        }
    }

    const int16_t *table;

    uint8_t octave = pgm_read_byte(&(notes_octaves[o->_note]));
    if (octave >= oscillator_blsquare_rows) {
        table = oscillator_sine;
    }
    else {
        switch (o->_waveform) {
        case OSCILLATOR_WAVEFORM_SQUARE:
            table = oscillator_blsquare[octave];
            break;

        case OSCILLATOR_WAVEFORM_SINE:
            table = oscillator_sine;
            break;

        case OSCILLATOR_WAVEFORM_TRIANGLE:
            table = oscillator_bltriangle[octave];
            break;

        case OSCILLATOR_WAVEFORM_SAW:
            table = oscillator_blsawtooth[octave];
            break;

        default:
            return 0;
        }
    }

    return pgm_read_word(&(table[o->_phase.pint]));
}
