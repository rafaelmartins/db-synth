/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <stdint.h>

typedef enum {
    OSCILLATOR_WAVEFORM_SQUARE,
    OSCILLATOR_WAVEFORM_SINE,
    OSCILLATOR_WAVEFORM_TRIANGLE,
    OSCILLATOR_WAVEFORM_SAW,
    OSCILLATOR_WAVEFORM__LAST,
} oscillator_waveform_t;

void oscillator_set_waveform(oscillator_waveform_t wf);
void oscillator_set_note(uint8_t n);
int16_t oscillator_get_sample(void);
