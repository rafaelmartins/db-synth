/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "phase.h"
#include "oscillator.h"
#include "oscillator-data.h"


int16_t
oscillator_get_sample(volatile oscillator_t *o)
{
    if (o == NULL || !o->_initialized)
        return 0;

    if (o->_note > oscillator_notes_last) {  // not running
        if (o->_note_next > oscillator_notes_last || o->_waveform_next >= OSCILLATOR_WAVEFORM__LAST)  // no note to play yet
            return 0;
        o->_note = o->_note_next;
        o->_note_next = 0xff;
        o->_waveform = o->_waveform_next;
        o->_waveform_next = OSCILLATOR_WAVEFORM__LAST;
        o->_phase.data = 0;
    }
    else if (phase_step(&o->_phase, pgm_read_dword(&oscillator_notes[o->_note]), oscillator_waveform_samples_per_cycle)) {
        if (o->_note_next <= oscillator_notes_last) {  // new note to play
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
    if (octave >= oscillator_wavetable_octaves) {
        table = oscillator_sine_wavetable;
    }
    else {
        switch (o->_waveform) {
        case OSCILLATOR_WAVEFORM_SQUARE:
            table = oscillator_square_wavetables[octave];
            break;

        case OSCILLATOR_WAVEFORM_SINE:
            table = oscillator_sine_wavetable;
            break;

        case OSCILLATOR_WAVEFORM_TRIANGLE:
            table = oscillator_triangle_wavetables[octave];
            break;

        case OSCILLATOR_WAVEFORM_SAW:
            table = oscillator_sawtooth_wavetables[octave];
            break;

        default:
            return 0;
        }
    }

    return pgm_read_word(&(table[o->_phase.pint]));
}
