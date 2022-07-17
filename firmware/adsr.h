/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <stdint.h>

void adsr_set_attack(uint8_t a);
void adsr_set_decay(uint8_t d);
void adsr_set_sustain(uint8_t s);
void adsr_set_release(uint8_t r);
// velocity is not part of adsr, but it is easier to optimize here
void adsr_set_velocity(uint8_t v);
void adsr_set_gate();
void adsr_unset_gate();
int16_t adsr_sample(int16_t in);
