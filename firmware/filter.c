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
#include "filter.h"
#include "filter-data.h"


void
filter_init(filter_t *f)
{
    if (f == NULL || f->_initialized)
        return;

    f->_initialized = true;
    f->_type = FILTER_TYPE_OFF;
    f->_cutoff = 0x7f;
    f->_prev = 0;
}


bool
filter_set_type(filter_t *f, filter_type_t t)
{
    if (f != NULL && f->_initialized && f->_type != t) {
        f->_type = t;
        return true;
    }
    return false;
}


bool
filter_set_cutoff(filter_t *f, uint8_t cutoff)
{
    if (f != NULL && f->_initialized && f->_cutoff != cutoff && cutoff < 0x80) {
        f->_cutoff = cutoff;
        return true;
    }
    return false;
}


int16_t
filter_get_sample(filter_t *f, int16_t in)
{
    if (f == NULL || !f->_initialized)
        return 0;

    uint16_t coefficients;

    switch (f->_type) {
    case FILTER_TYPE_LOW_PASS:
        coefficients = (uint16_t) &filter_lp[f->_cutoff].a1;
        break;

    case FILTER_TYPE_HIGH_PASS:
        coefficients = (uint16_t) &filter_hp[f->_cutoff].a1;
        break;

    case FILTER_TYPE_OFF:
    case FILTER_TYPE__LAST:
        return in;
    }

    int16_t rv;
    asm volatile (
        "lpm r23, Z+"    "\n\t"  // tmp = coefficients++
        "muls r23, %B2"  "\n\t"  // $result = a1 * _prev[h] (signed * signed)
        "movw %A0, r0"   "\n\t"  // rv = $result
        "mulsu r23, %A2" "\n\t"  // $result = a1 * _prev[l] (signed * unsigned)
        "clr r0"         "\n\t"  // $r0 = 0
        "sbc %B0, r0"    "\n\t"  // rv[h] -= 0 + $carry
        "add %A0, r1"    "\n\t"  // rv[l] += $result[h]
        "adc %B0, r0"    "\n\t"  // rv[h] += 0 + $carry
        "lpm r23, Z+"    "\n\t"  // tmp = coefficients++
        "muls r23, %B3"  "\n\t"  // $result = tmp * in[h] (signed * signed)
        "add %A0, r0"    "\n\t"  // rv[l] += $result[l]
        "adc %B0, r1"    "\n\t"  // rv[h] += $result[h] + $carry
        "mulsu r23, %A3" "\n\t"  // $result = tmp * in[l] (signed * unsigned)
        "clr r0"         "\n\t"  // $r0 = 0
        "sbc %B0, r0"    "\n\t"  // rv[h] -= 0 + $carry
        "add %A0, r1"    "\n\t"  // rv[l] += $result[h]
        "adc %B0, r0"    "\n\t"  // rv[h] += 0 + $carry
        "lpm r23, Z"     "\n\t"  // tmp = coefficients
        "muls r23, %B2"  "\n\t"  // $result = tmp * _prev[h] (signed * signed)
        "add %A0, r0"    "\n\t"  // rv[l] += $result[l]
        "adc %B0, r1"    "\n\t"  // rv[h] += $result[h] + $carry
        "mulsu r23, %A2" "\n\t"  // $result = tmp * _prev[l] (signed * unsigned)
        "clr r0"         "\n\t"  // $r0 = 0
        "sbc %B0, r0"    "\n\t"  // rv[h] -= 0 + $carry
        "add %A0, r1"    "\n\t"  // rv[l] += $result[h]
        "adc %B0, r0"    "\n\t"  // rv[h] += 0 + $carry
        "lsl %A0"        "\n\t"  // rv[l] = (rv[l] << 1)
        "rol %B0"        "\n\t"  // rv[h] = (rv[h] << 1) + $carry
        "clr r1"         "\n\t"  // $r1 = 0 (avr-libc convention)
        : "=d" (rv), "=z" (coefficients)
        : "a" (f->_prev), "a" (in), "1" (coefficients)
        : "r23"
    );

    f->_prev = rv;
    return rv;
}
