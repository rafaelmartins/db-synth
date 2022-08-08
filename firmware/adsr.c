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
#include "phase.h"
#include "adsr.h"
#include "adsr-data.h"


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


void
adsr_set_gate(volatile adsr_t *a)
{
    _set_state(a, ADSR_STATE_ATTACK);
}


void
adsr_unset_gate(volatile adsr_t *a)
{
    _set_state(a, ADSR_STATE_RELEASE);
}


int16_t
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
