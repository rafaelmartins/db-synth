/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "phase.h"
#include "oscillator.h"
#include "oscillator-data.h"

static volatile phase_t phase = {0};
static volatile oscillator_waveform_t waveform = OSCILLATOR_WAVEFORM__LAST;
static volatile oscillator_waveform_t waveform_next = OSCILLATOR_WAVEFORM__LAST;
static volatile uint8_t note = 0xff;
static volatile uint8_t note_next = 0xff;


void
oscillator_set_waveform(oscillator_waveform_t wf)
{
    waveform_next = wf;
}


void
oscillator_set_note(uint8_t n)
{
    note_next = n;
}


int16_t
oscillator_get_sample(void)
{
    if (note > notes_last) {  // not running
        if (note_next > notes_last || waveform_next >= OSCILLATOR_WAVEFORM__LAST)  // no note to play yet
            return 0;
        note = note_next;
        note_next = 0xff;
        waveform = waveform_next;
        waveform_next = OSCILLATOR_WAVEFORM__LAST;
        phase.data = 0;
    }
    else if (phase_step(&phase, &notes[note], waveform_samples_per_cycle)) {
        if (note_next <= notes_last) {  // new note to play
            note = note_next;
            note_next = 0xff;
        }
        if (waveform_next < OSCILLATOR_WAVEFORM__LAST) {  // new waveform to set
            waveform = waveform_next;
            waveform_next = OSCILLATOR_WAVEFORM__LAST;
        }
    }

    const int16_t *table;

    uint8_t octave = note / 12;
    if (octave >= wavetable_octaves) {
        table = sine_wavetable;
    }
    else {
        switch (waveform) {
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

    return pgm_read_word(&(table[phase.pint]));
}
