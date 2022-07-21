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

#include "oled-font.h"

#define oled_screen_width 128
#define oled_screen_height 68
#define oled_chars_per_line (oled_screen_width / (oled_font_width + 1))
#define oled_lines (oled_screen_height / 8)  // info from ssd1306 datasheet

typedef enum {
    OLED_HALIGN_LEFT = 1,
    OLED_HALIGN_RIGHT,
    OLED_HALIGN_CENTER,
} oled_halign_t;

bool oled_clear_line(uint8_t line);
bool oled_clear(void);
bool oled_init(void);
bool oled_line(uint8_t line, const char *str, oled_halign_t align);
bool oled_task(void);
