/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "phase.h"
#include "adsr-data.h"

typedef enum {
    ADSR_STATE_OFF,
    ADSR_STATE_ATTACK,
    ADSR_STATE_DECAY,
    ADSR_STATE_SUSTAIN,
    ADSR_STATE_RELEASE,
} adsr_state_t;

typedef enum {
    ADSR_TYPE_EXPONENTIAL,
    ADSR_TYPE_LINEAR,
    ADSR_TYPE__LAST,
} adsr_type_t;

typedef struct {
    bool _initialized;
    adsr_state_t _state;
    adsr_type_t _type;
    uint8_t _attack;
    uint8_t _decay;
    uint8_t _sustain;
    uint8_t _sustain_level;
    uint8_t _release;
    uint8_t _level;
    uint8_t _range_start;
    uint8_t _range_end;
    phase_t _phase;
} adsr_t;


static inline void
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
        a->_range_end = adsr_amplitude;
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

    a->_phase.data = 0;
}


static inline bool
adsr_set_type(adsr_t *a, adsr_type_t t)
{
    if (a != NULL && a->_initialized && a->_type != t) {
        a->_type = t;
        return true;
    }
    return false;
}


static inline bool
adsr_set_attack(adsr_t *a, uint8_t attack)
{
    if (a != NULL && a->_initialized && a->_attack != attack && attack < 0x80) {
        a->_attack = attack;
        return true;
    }
    return false;
}


static inline bool
adsr_set_decay(adsr_t *a, uint8_t decay)
{
    if (a != NULL && a->_initialized && a->_decay != decay && decay < 0x80) {
        a->_decay = decay;
        return true;
    }
    return false;
}


static inline bool
adsr_set_sustain(adsr_t *a, uint8_t sustain)
{
    if (a != NULL && a->_initialized && a->_sustain != sustain && sustain < 0x80) {
        a->_sustain = sustain;
        a->_sustain_level = sustain << 1;
        return true;
    }
    return false;
}


static inline bool
adsr_set_release(adsr_t *a, uint8_t release)
{
    if (a != NULL && a->_initialized && a->_release != release && release < 0x80) {
        a->_release = release;
        return true;
    }
    return false;
}


static inline void
adsr_set_gate(adsr_t *a)
{
    _set_state(a, ADSR_STATE_ATTACK);
}


static inline void
adsr_unset_gate(adsr_t *a, bool force)
{
    _set_state(a, force ? ADSR_STATE_OFF: ADSR_STATE_RELEASE);
}


static inline uint8_t
adsr_get_sample_level(adsr_t *a)
{
    if (a == NULL || !a->_initialized)
        return 0;

    const uint8_t *table = NULL;

    switch (a->_state) {
    case ADSR_STATE_ATTACK:
        if (phase_step(&a->_phase, pgm_read_dword(&adsr_time_steps[a->_attack]), adsr_samples_per_cycle))
            _set_state(a, ADSR_STATE_DECAY);
        table = a->_type == ADSR_TYPE_LINEAR ? adsr_linear_curve : adsr_attack_curve;
        break;

    case ADSR_STATE_DECAY:
        if (phase_step(&a->_phase, pgm_read_dword(&adsr_time_steps[a->_decay]), adsr_samples_per_cycle))
            _set_state(a, ADSR_STATE_SUSTAIN);
        else
            table = a->_type == ADSR_TYPE_LINEAR ? adsr_linear_curve : adsr_decay_release_curve;
        break;

    case ADSR_STATE_RELEASE:
        if (phase_step(&a->_phase, pgm_read_dword(&adsr_time_steps[a->_release]), adsr_samples_per_cycle)) {
            _set_state(a, ADSR_STATE_OFF);
            return 0;
        }
        table = a->_type == ADSR_TYPE_LINEAR ? adsr_linear_curve : adsr_decay_release_curve;
        break;

    case ADSR_STATE_SUSTAIN:
        break;

    case ADSR_STATE_OFF:
        return 0;
    }

    uint8_t balance = table != NULL ? pgm_read_byte(&(table[a->_phase.pint])) : adsr_amplitude;

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
