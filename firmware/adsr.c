/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include "phase.h"
#include "adsr.h"
#include "adsr-data.h"

typedef enum {
    ADSR_STATE_OFF,
    ADSR_STATE_ATTACK,
    ADSR_STATE_DECAY,
    ADSR_STATE_SUSTAIN,
    ADSR_STATE_RELEASE,
} adsr_state_t;

static volatile uint8_t attack = 0;
static volatile uint8_t decay = 0;
static volatile uint8_t sustain = 0xff;
static volatile uint8_t release = 0;
static volatile uint8_t velocity = 0xfe;
static volatile uint8_t level = 0;
static volatile uint8_t range_start = 0;
static volatile uint8_t range_end = 0;
static volatile phase_t phase;
static volatile adsr_state_t state = ADSR_STATE_OFF;


static inline void
set_state(adsr_state_t s)
{
    state = s;
    range_start = level;

    switch (state) {
    case ADSR_STATE_ATTACK:
        range_end = adsr_amplitude;
        break;

    case ADSR_STATE_DECAY:
        range_end = sustain;
        break;

    case ADSR_STATE_SUSTAIN:
        range_end = level;
        break;

    case ADSR_STATE_RELEASE:
        range_end = 0;
        break;

    case ADSR_STATE_OFF:
        break;
    }

    phase.data = 0;
}


void
adsr_set_attack(uint8_t a)
{
    attack = a;
}


void
adsr_set_decay(uint8_t d)
{
    decay = d;
}


void
adsr_set_sustain(uint8_t s)
{
    sustain = s;
}


void
adsr_set_release(uint8_t r)
{
    release = r;
}


void
adsr_set_velocity(uint8_t v)
{
    if (v > 127)
        v = 127;
    velocity = 2 * v;
}


void
adsr_set_gate()
{
    set_state(ADSR_STATE_ATTACK);
}


void
adsr_unset_gate()
{
    set_state(ADSR_STATE_RELEASE);
}


int16_t
adsr_sample(int16_t in)
{
    const uint8_t *table = NULL;

    switch (state) {
    case ADSR_STATE_ATTACK:
        if (phase_step(&phase, &times[attack].step, adsr_samples_per_cycle))
            set_state(ADSR_STATE_DECAY);
        table = attack_curve;
        break;

    case ADSR_STATE_DECAY:
        if (phase_step(&phase, &times[decay].step, adsr_samples_per_cycle))
            set_state(ADSR_STATE_SUSTAIN);
        else
            table = decay_release_curve;
        break;

    case ADSR_STATE_RELEASE:
        if (phase_step(&phase, &times[release].step, adsr_samples_per_cycle)) {
            set_state(ADSR_STATE_OFF);
            return 0;
        }
        table = decay_release_curve;
        break;

    case ADSR_STATE_SUSTAIN:
        break;

    case ADSR_STATE_OFF:
        return 0;
    }

    uint8_t balance = table != NULL ? pgm_read_byte(&(table[phase.pint])) : adsr_amplitude;

    uint16_t rv;
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
        : "=&a" (rv), "=&r" (level)
        : "a" (in), "r" (range_start), "r" (range_end), "r" (balance), "r" (velocity)
    );
    return rv;
}
