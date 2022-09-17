/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <stdint.h>

typedef union {
    uint32_t data;
    struct {
        uint16_t pfrac;
        uint16_t pint;
    };
} phase_t;

bool phase_step(phase_t *p, uint32_t s, uint16_t table_len);
