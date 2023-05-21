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
    if (f != NULL && f->_initialized && f->_cutoff != cutoff && cutoff < filter_lowpass_1pole_coefficients_len) {
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

    uint8_t a1;
    uint8_t b0;
    uint8_t b1;

    switch (f->_type) {
    case FILTER_TYPE_LOW_PASS:
        a1 = filter_lowpass_1pole_coefficients[f->_cutoff].a1;
        b0 = filter_lowpass_1pole_coefficients[f->_cutoff].b0;
        b1 = filter_lowpass_1pole_coefficients[f->_cutoff].b1;
        break;

    case FILTER_TYPE_HIGH_PASS:
        a1 = filter_highpass_1pole_coefficients[f->_cutoff].a1;
        b0 = filter_highpass_1pole_coefficients[f->_cutoff].b0;
        b1 = filter_highpass_1pole_coefficients[f->_cutoff].b1;
        break;

    case FILTER_TYPE_OFF:
    case FILTER_TYPE__LAST:
        return in;
    }

    int16_t rv;
    asm volatile (
        "muls %3, %B1"  "\n\t"  // $result = a1 * _prev[h] (signed * signed)
        "movw %A0, r0"  "\n\t"  // rv = $result
        "mulsu %3, %A1" "\n\t"  // $result = a1 * _prev[l] (signed * unsigned)
        "clr r0"        "\n\t"  // $r0 = 0
        "sbc %B0, r0"   "\n\t"  // rv[h] -= 0 + $carry
        "add %A0, r1"   "\n\t"  // rv[l] += $result[h]
        "adc %B0, r0"   "\n\t"  // rv[h] += 0 + $carry
        "muls %4, %B2"  "\n\t"  // $result = b0 * in[h] (signed * signed)
        "add %A0, r0"   "\n\t"  // rv[l] += $result[l]
        "adc %B0, r1"   "\n\t"  // rv[h] += $result[h] + $carry
        "mulsu %4, %A2" "\n\t"  // $result = b0 * in[l] (signed * unsigned)
        "clr r0"        "\n\t"  // $r0 = 0
        "sbc %B0, r0"   "\n\t"  // rv[h] -= 0 + $carry
        "add %A0, r1"   "\n\t"  // rv[l] += $result[h]
        "adc %B0, r0"   "\n\t"  // rv[h] += 0 + $carry
        "muls %5, %B1"  "\n\t"  // $result = b1 * _prev[h] (signed * signed)
        "add %A0, r0"   "\n\t"  // rv[l] += $result[l]
        "adc %B0, r1"   "\n\t"  // rv[h] += $result[h] + $carry
        "mulsu %5, %A1" "\n\t"  // $result = b1 * _prev[l] (signed * unsigned)
        "clr r0"        "\n\t"  // $r0 = 0
        "sbc %B0, r0"   "\n\t"  // rv[h] -= 0 + $carry
        "add %A0, r1"   "\n\t"  // rv[l] += $result[h]
        "adc %B0, r0"   "\n\t"  // rv[h] += 0 + $carry
        "lsl %A0"       "\n\t"  // rv[l] = (rv[l] << 1)
        "rol %B0"       "\n\t"  // rv[h] = (rv[h] << 1) + $carry
        "clr r1"        "\n\t"  // $r1 = 0 (avr-libc convention)
        : "=d" (rv)
        : "a" (f->_prev), "a" (in), "a" (a1), "a" (b0), "a" (b1)
    );

    f->_prev = rv;
    return rv;
}
