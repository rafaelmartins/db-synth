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


static inline bool
oscillator_set_waveform(volatile oscillator_t *o, oscillator_waveform_t wf)
{
    if (o != NULL && o->_initialized && o->_waveform != wf) {
        o->_waveform_next = wf;
        return true;
    }
    return false;
}


static inline void
oscillator_set_note(volatile oscillator_t *o, uint8_t n)
{
    if (o != NULL && o->_initialized)
        o->_note_next = n;
}


int16_t oscillator_get_sample(volatile oscillator_t *o);
