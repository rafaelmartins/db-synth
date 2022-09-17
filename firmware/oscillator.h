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
#include "phase.h"

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

void oscillator_init(oscillator_t *o);
bool oscillator_set_waveform(oscillator_t *o, oscillator_waveform_t wf);
void oscillator_set_note(oscillator_t *o, uint8_t n);
int16_t oscillator_get_sample(oscillator_t *o);
