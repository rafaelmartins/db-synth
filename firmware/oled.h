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

#define oled_i2c_address 0x3c
#define oled_screen_width 128
#define oled_screen_height 68
#define oled_chars_per_line (oled_screen_width / (oled_font_width + 1))
#define oled_lines (oled_screen_height / 8)  // info from ssd1306 datasheet

typedef enum {
    OLED_HALIGN_LEFT = 1,
    OLED_HALIGN_RIGHT,
    OLED_HALIGN_CENTER,
} oled_halign_t;

typedef struct {
    bool _initialized;
    bool _polling;
    uint8_t _current_line;
    uint8_t _current_data;
    uint8_t _current_char;

    enum {
        OLED_TASK_STATE_ADDR1,
        OLED_TASK_STATE_RENDER1,
        OLED_TASK_STATE_RENDER2,
        OLED_TASK_STATE_RENDER3,
        OLED_TASK_STATE_RENDER4,
        OLED_TASK_STATE_STOP1,
        OLED_TASK_STATE_ADDR2,
        OLED_TASK_STATE_DATA1,
        OLED_TASK_STATE_DATAN,
        OLED_TASK_STATE_STOP2,
        OLED_TASK_STATE_END,
    } _task_state;

    struct {
        enum {
            LINE_STATE_FREE,
            LINE_STATE_PENDING,
            LINE_STATE_RENDERING,
        } state;
        uint8_t data[oled_chars_per_line + 1];
    } _lines[8];
} oled_t;

bool oled_init(oled_t *o);
bool oled_line(oled_t *o, uint8_t line, const char *str, oled_halign_t align);
bool oled_task(oled_t *o);
