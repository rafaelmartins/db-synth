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

typedef union {
    uint32_t data;
    struct {
        uint16_t pfrac;
        uint16_t pint;
    };
} phase_t;


static inline bool
phase_step(volatile phase_t *p, const phase_t *s, uint16_t table_len)
{
    if (p == NULL || s == NULL)
        return false;

    p->data += s->data;
    if (p->pint >= table_len) {
        p->pint -= table_len;
        return true;
    }
    return false;
}
