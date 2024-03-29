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
#include "adsr.h"
#include "adsr-data.h"


void
adsr_init(adsr_t *a)
{
    if (a == NULL || a->_initialized)
        return;

    a->_initialized = true;
    a->_state = ADSR_STATE_OFF;
    a->_sustain = 0x7f;
    a->_sustain_level = a->_sustain << 1;
}


static inline void
_set_state(adsr_t *a, adsr_state_t s)
{
    if (a == NULL || !a->_initialized)
        return;

    a->_state = s;
    a->_range_start = a->_level;

    switch (a->_state) {
    case ADSR_STATE_ATTACK:
        a->_range_end = adsr_sample_amplitude;
        break;

    case ADSR_STATE_DECAY:
        a->_range_end = a->_sustain_level;
        break;

    case ADSR_STATE_SUSTAIN:
        a->_range_end = a->_level;
        break;

    case ADSR_STATE_RELEASE:
        a->_range_end = 0;
        break;

    case ADSR_STATE_OFF:
        break;
    }

    a->_time.data = 0;
}


bool
adsr_set_type(adsr_t *a, adsr_type_t t)
{
    if (a != NULL && a->_initialized && a->_type != t) {
        a->_type = t;
        return true;
    }
    return false;
}


bool
adsr_set_attack(adsr_t *a, uint8_t attack)
{
    if (a != NULL && a->_initialized && a->_attack != attack && attack < 0x80) {
        a->_attack = attack;
        return true;
    }
    return false;
}


bool
adsr_set_decay(adsr_t *a, uint8_t decay)
{
    if (a != NULL && a->_initialized && a->_decay != decay && decay < 0x80) {
        a->_decay = decay;
        return true;
    }
    return false;
}


bool
adsr_set_sustain(adsr_t *a, uint8_t sustain)
{
    if (a != NULL && a->_initialized && a->_sustain != sustain && sustain < 0x80) {
        a->_sustain = sustain;
        a->_sustain_level = sustain << 1;
        return true;
    }
    return false;
}


bool
adsr_set_release(adsr_t *a, uint8_t release)
{
    if (a != NULL && a->_initialized && a->_release != release && release < 0x80) {
        a->_release = release;
        return true;
    }
    return false;
}


void
adsr_set_gate(adsr_t *a)
{
    _set_state(a, ADSR_STATE_ATTACK);
}


void
adsr_unset_gate(adsr_t *a, bool force)
{
    _set_state(a, force ? ADSR_STATE_OFF: ADSR_STATE_RELEASE);
}


static bool
time_step(adsr_time_t *t, uint8_t idx)
{
    if (t == NULL)
        return false;

    t->data += adsr_time_steps[idx];
    if (t->pint >= adsr_time_steps_len) {
        t->pint -= adsr_time_steps_len;
        return true;
    }
    return false;
}


uint8_t
adsr_get_sample_level(adsr_t *a)
{
    if (a == NULL || !a->_initialized)
        return 0;

    const uint8_t *table = NULL;

    switch (a->_state) {
    case ADSR_STATE_ATTACK:
        if (time_step(&a->_time, a->_attack))
            _set_state(a, ADSR_STATE_DECAY);
        table = a->_type == ADSR_TYPE_LINEAR ? adsr_curve_linear : adsr_curve_as3310_attack;
        break;

    case ADSR_STATE_DECAY:
        if (time_step(&a->_time, a->_decay))
            _set_state(a, ADSR_STATE_SUSTAIN);
        else
            table = a->_type == ADSR_TYPE_LINEAR ? adsr_curve_linear : adsr_curve_as3310_decay_release;
        break;

    case ADSR_STATE_RELEASE:
        if (time_step(&a->_time, a->_release)) {
            _set_state(a, ADSR_STATE_OFF);
            return 0;
        }
        table = a->_type == ADSR_TYPE_LINEAR ? adsr_curve_linear : adsr_curve_as3310_decay_release;
        break;

    case ADSR_STATE_SUSTAIN:
        break;

    case ADSR_STATE_OFF:
        return 0;
    }

    uint8_t balance = table != NULL ? table[a->_time.pint] : adsr_sample_amplitude;

    uint16_t tmp;
    asm volatile (
        "mul %2, %3"   "\n\t"  // $result = range_end * balance (unsigned multiplication)
        "movw %A0, r0" "\n\t"  // tmp = $result
        "com %3"       "\n\t"  // balance = balance complement to 0xff
        "mul %1, %3"   "\n\t"  // $result = range_start * balance (unsigned multiplication)
        "add %A0, r0"  "\n\t"  // tmp[l] += $result[l]
        "adc %B0, r1"  "\n\t"  // tmp[h] += $result[h] + $carry
        "mov %A0, %B0" "\n\t"  // tmp[l] = tmp[h]
        "clr r1"       "\n\t"  // $r1 = 0 (avr-libc convention)
        : "=&r" (tmp)
        : "r" (a->_range_start), "r" (a->_range_end), "r" (balance)
    );
    a->_level = tmp;
    return a->_level;
}
