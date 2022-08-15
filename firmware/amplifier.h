/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <stdint.h>


static inline int16_t
amplifier_get_sample(int16_t in, uint8_t level1, uint8_t level2)
{
    int16_t rv;
    asm volatile (
        "mul %2, %3"     "\n\t"  // $result = level1 * level2 (unsigned multiplication)
        "mov %B0, r1"    "\n\t"  // rv[h] ($tmp) = $result[h]
        "mul %A1, %B0"   "\n\t"  // $result = in[l] * rv[h] ($tmp) (unsigned multiplication)
        "mov %A0, r1"    "\n\t"  // rv[l] = $result[h]
        "mulsu %B1, %B0" "\n\t"  // $result = in[h] * rv[h] ($tmp) (signed multiplication)
        "add %A0, r0"    "\n\t"  // rv[l] += $result[l]
        "clr %B0"        "\n\t"  // rv[h] = 0
        "adc %B0, r1"    "\n\t"  // rv[h] += $result[h] + $carry
        "clr r1"         "\n\t"  // $r1 = 0 (avr-libc convention)
        : "=&a" (rv)
        : "a" (in), "r" (level1), "r" (level2)
    );
    return rv;
}
