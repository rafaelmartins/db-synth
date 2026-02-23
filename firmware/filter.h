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

typedef enum {
    FILTER_TYPE_OFF,
    FILTER_TYPE_LOW_PASS,
    FILTER_TYPE_HIGH_PASS,
    FILTER_TYPE__LAST,
} filter_type_t;

typedef struct {
    bool _initialized;
    filter_type_t _type;
    uint8_t _cutoff;
    int16_t _prev_out;
    int16_t _prev_in;
} filter_t;

void filter_init(filter_t *f);
bool filter_set_type(filter_t *f, filter_type_t t);
bool filter_set_cutoff(filter_t *f, uint8_t cutoff);
int16_t filter_get_sample(filter_t *f, int16_t in);
