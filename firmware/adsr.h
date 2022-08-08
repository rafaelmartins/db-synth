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
#include "phase.h"

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


void adsr_set_gate(volatile adsr_t *a);
void adsr_unset_gate(volatile adsr_t *a);
int16_t adsr_get_sample(volatile adsr_t *a, int16_t in);
