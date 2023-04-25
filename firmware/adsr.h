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

typedef union {
    uint32_t data : 24;
    struct {
        uint16_t pfrac;
        uint8_t pint;
    };
} adsr_time_t;

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
    adsr_time_t _time;
} adsr_t;

void adsr_init(adsr_t *a);
bool adsr_set_type(adsr_t *a, adsr_type_t t);
bool adsr_set_attack(adsr_t *a, uint8_t attack);
bool adsr_set_decay(adsr_t *a, uint8_t decay);
bool adsr_set_sustain(adsr_t *a, uint8_t sustain);
bool adsr_set_release(adsr_t *a, uint8_t release);
void adsr_set_gate(adsr_t *a);
void adsr_unset_gate(adsr_t *a, bool force);
uint8_t adsr_get_sample_level(adsr_t *a);
