/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "phase.h"


bool
phase_step(phase_t *p, uint32_t s, uint16_t table_len)
{
    if (p == NULL)
        return false;

    p->data += s;
    if (p->pint >= table_len) {
        p->pint -= table_len;
        return true;
    }
    return false;
}
