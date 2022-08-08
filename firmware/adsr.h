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

typedef struct {
    bool _initialized;
    adsr_state_t _state;
    uint8_t _attack;
    uint8_t _decay;
    uint8_t _sustain;
    uint8_t _release;
    // velocity is not part of adsr, but it is easier to optimize here
    uint8_t _velocity;
    uint8_t _level;
    uint8_t _range_start;
    uint8_t _range_end;
    phase_t _phase;
} adsr_t;


static inline void
adsr_init(volatile adsr_t *a)
{
    if (a == NULL || a->_initialized)
        return;

    a->_initialized = true;
    a->_state = ADSR_STATE_OFF;
    a->_sustain = 0xff;
    a->_velocity = 0xfe;
}


static inline void
_set_state(volatile adsr_t *a, adsr_state_t s)
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
        a->_range_end = a->_sustain;
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
adsr_set_attack(volatile adsr_t *a, uint8_t attack)
{
    if (a != NULL && a->_initialized && a->_attack != attack) {
        a->_attack = attack;
        return true;
    }
    return false;
}


static inline bool
adsr_set_decay(volatile adsr_t *a, uint8_t decay)
{
    if (a != NULL && a->_initialized && a->_decay != decay) {
        a->_decay = decay;
        return true;
    }
    return false;
}


static inline bool
adsr_set_sustain(volatile adsr_t *a, uint8_t sustain)
{
    if (a != NULL && a->_initialized && a->_sustain != sustain) {
        a->_sustain = sustain;
        return true;
    }
    return false;
}


static inline bool
adsr_set_release(volatile adsr_t *a, uint8_t release)
{
    if (a != NULL && a->_initialized && a->_release != release) {
        a->_release = release;
        return true;
    }
    return false;
}


static inline void
adsr_set_velocity(volatile adsr_t *a, uint8_t v)
{
    if (a == NULL || !a->_initialized)
        return;

    if (v > 127)
        v = 127;
    a->_velocity = 2 * v;
}


static inline void
adsr_set_gate(volatile adsr_t *a)
{
    _set_state(a, ADSR_STATE_ATTACK);
}


static inline void
adsr_unset_gate(volatile adsr_t *a)
{
    _set_state(a, ADSR_STATE_RELEASE);
}


static inline int16_t
adsr_get_sample(volatile adsr_t *a, int16_t in)
{
    if (a == NULL || !a->_initialized)
        return 0;

    const uint8_t *table = NULL;

    switch (a->_state) {
    case ADSR_STATE_ATTACK:
        if (phase_step(&a->_phase, pgm_read_dword(&adsr_time_steps[a->_attack]), adsr_samples_per_cycle))
            _set_state(a, ADSR_STATE_DECAY);
        table = adsr_attack_curve;
        break;

    case ADSR_STATE_DECAY:
        if (phase_step(&a->_phase, pgm_read_dword(&adsr_time_steps[a->_decay]), adsr_samples_per_cycle))
            _set_state(a, ADSR_STATE_SUSTAIN);
        else
            table = adsr_decay_release_curve;
        break;

    case ADSR_STATE_RELEASE:
        if (phase_step(&a->_phase, pgm_read_dword(&adsr_time_steps[a->_release]), adsr_samples_per_cycle)) {
            _set_state(a, ADSR_STATE_OFF);
            return 0;
        }
        table = adsr_decay_release_curve;
        break;

    case ADSR_STATE_SUSTAIN:
        break;

    case ADSR_STATE_OFF:
        return 0;
    }

    uint8_t balance = table != NULL ? pgm_read_byte(&(table[a->_phase.pint])) : adsr_amplitude;

    int16_t rv;
    asm volatile (
        "mul %4, %5"     "\n\t"  // $result = range_end * balance (unsigned multiplication)
        "movw %A0, r0"   "\n\t"  // rv = $result
        "com %5"         "\n\t"  // balance = balance complement to 0xff
        "mul %3, %5"     "\n\t"  // $result = range_start * balance (unsigned multiplication)
        "add %A0, r0"    "\n\t"  // rv[l] += $result[l]
        "adc %B0, r1"    "\n\t"  // rv[h] += $result[h] + $carry
        "mov %1, %B0"    "\n\t"  // level = rv[h]
        "mul %B0, %6"    "\n\t"  // $result = rv[h] (level) * velocity (unsigned multiplication)
        "mov %B0, r1"    "\n\t"  // rv[h] ($tmp) = $result[h]
        "mul %A2, %B0"   "\n\t"  // $result = in[l] * rv[h] ($tmp) (unsigned multiplication)
        "mov %A0, r1"    "\n\t"  // rv[l] = $result[h]
        "mulsu %B2, %B0" "\n\t"  // $result = in[h] * rv[h] ($tmp) (signed multiplication)
        "add %A0, r0"    "\n\t"  // rv[l] += $result[l]
        "clr %B0"        "\n\t"  // rv[h] = 0
        "adc %B0, r1"    "\n\t"  // rv[h] += $result[h] + $carry
        "clr r1"         "\n\t"  // $r1 = 0 (avr-libc convention)
        : "=&a" (rv), "=&r" (a->_level)
        : "a" (in), "r" (a->_range_start), "r" (a->_range_end), "r" (balance), "r" (a->_velocity)
    );
    return rv;
}
