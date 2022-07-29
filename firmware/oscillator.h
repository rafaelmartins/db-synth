/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <stdint.h>
#include <avr/pgmspace.h>
#include "phase.h"
#include "oscillator-data.h"

typedef enum {
    OSCILLATOR_WAVEFORM_SQUARE,
    OSCILLATOR_WAVEFORM_SINE,
    OSCILLATOR_WAVEFORM_TRIANGLE,
    OSCILLATOR_WAVEFORM_SAW,
    OSCILLATOR_WAVEFORM__LAST,
} oscillator_waveform_t;

typedef struct {
    bool _initialized;
    phase_t _phase;
    oscillator_waveform_t _waveform;
    oscillator_waveform_t _waveform_next;
    uint8_t _note;
    uint8_t _note_next;
} oscillator_t;


static inline void
oscillator_init(volatile oscillator_t *o)
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


static inline void
oscillator_set_waveform(volatile oscillator_t *o, oscillator_waveform_t wf)
{
    if (o != NULL && o->_initialized)
        o->_waveform_next = wf;
}


static inline void
oscillator_set_note(volatile oscillator_t *o, uint8_t n)
{
    if (o != NULL && o->_initialized)
        o->_note_next = n;
}


static inline int16_t
oscillator_get_sample(volatile oscillator_t *o)
{
    if (o == NULL || !o->_initialized)
        return 0;

    if (o->_note > notes_last) {  // not running
        if (o->_note_next > notes_last || o->_waveform_next >= OSCILLATOR_WAVEFORM__LAST)  // no note to play yet
            return 0;
        o->_note = o->_note_next;
        o->_note_next = 0xff;
        o->_waveform = o->_waveform_next;
        o->_waveform_next = OSCILLATOR_WAVEFORM__LAST;
        o->_phase.data = 0;
    }
    else if (phase_step(&o->_phase, &notes[o->_note], waveform_samples_per_cycle)) {
        if (o->_note_next <= notes_last) {  // new note to play
            o->_note = o->_note_next;
            o->_note_next = 0xff;
        }
        if (o->_waveform_next < OSCILLATOR_WAVEFORM__LAST) {  // new waveform to set
            o->_waveform = o->_waveform_next;
            o->_waveform_next = OSCILLATOR_WAVEFORM__LAST;
        }
    }

    const int16_t *table;

    uint8_t octave = o->_note / 12;
    if (octave >= wavetable_octaves) {
        table = sine_wavetable;
    }
    else {
        switch (o->_waveform) {
        case OSCILLATOR_WAVEFORM_SQUARE:
            table = square_wavetables[octave];
            break;

        case OSCILLATOR_WAVEFORM_SINE:
            table = sine_wavetable;
            break;

        case OSCILLATOR_WAVEFORM_TRIANGLE:
            table = triangle_wavetables[octave];
            break;

        case OSCILLATOR_WAVEFORM_SAW:
            table = sawtooth_wavetables[octave];
            break;

        default:
            return 0;
        }
    }

    return pgm_read_word(&(table[o->_phase.pint]));
}
