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
#include <string.h>
#include <avr/io.h>
#include "oled-data.h"
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
    bool _waiting;
    bool _stop;
    uint8_t _current_page;
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
        uint8_t data[oled_chars_per_line + 1];
        uint8_t start;
        bool end;
        bool rendered;
    } _lines[8];
} oled_t;


static inline bool
oled_clear_line(volatile oled_t *o, uint8_t line)
{
    if (o == NULL || line >= oled_lines)
        return false;

    o->_lines[line].data[0] = 0;
    o->_lines[line].start = 0;
    o->_lines[line].end = false;
    o->_lines[line].rendered = false;
    return true;
}


static inline bool
oled_clear(volatile oled_t *o)
{
    for (uint8_t i = 0; i < oled_lines; i++)
        if (!oled_clear_line(o, i))
            return false;
    return true;
}


static inline bool
_twi_wait(void)
{
    while (true) {
        if (TWI0.MSTATUS & TWI_WIF_bm)
            return !(TWI0.MSTATUS & TWI_RXACK_bm);
        else if(TWI0.MSTATUS & (TWI_BUSERR_bm | TWI_ARBLOST_bm))
            return false;
    }
    return false;
}


static inline bool
oled_init(volatile oled_t *o)
{
    if (o == NULL || o->_initialized)
        return false;

    TWI0.CTRLA = TWI_INPUTLVL_I2C_gc | TWI_SDASETUP_4CYC_gc | TWI_SDAHOLD_OFF_gc | TWI_FMPEN_OFF_gc;
    TWI0.MBAUD = oled_twi_mbaud;
    TWI0.MCTRLA = TWI_ENABLE_bm;
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;

    TWI0.MADDR = (oled_i2c_address << 1);
    if (!_twi_wait())
        return false;

    // Co = 0; D/|C = 0
    TWI0.MDATA = 0x00;
    if (!_twi_wait())
        return false;

    // set segment remap (reverse direction)
    TWI0.MDATA = 0xA1;
    if (!_twi_wait())
        return false;

    // set COM output scan direction (COM[N-1] to COM0)
    TWI0.MDATA = 0xC8;
    if (!_twi_wait())
        return false;

    // charge pump setting (enable during display on)
    TWI0.MDATA = 0x8D;
    if (!_twi_wait())
        return false;
    TWI0.MDATA = 0x14;
    if (!_twi_wait())
        return false;

    // set COM output scan direction (COM[N-1] to COM0)
    TWI0.MDATA = 0xAF;
    if (!_twi_wait())
        return false;

    TWI0.MCTRLB = TWI_MCMD_STOP_gc;

    o->_initialized = true;

    return oled_clear(o);
}


static inline bool
oled_line(volatile oled_t *o, uint8_t line, const char *str, oled_halign_t align)
{
    if (o == NULL || !o->_initialized)
        return false;

    if (str == NULL)
        return false;

    if (line >= oled_lines)
        return false;

    strncpy((char*) o->_lines[line].data, str, sizeof(o->_lines[line].data));
    o->_lines[line].end = false;
    o->_lines[line].rendered = false;

    size_t len = strlen(str) * (oled_font_width + 1);

    switch (align) {
    case OLED_HALIGN_LEFT:
        o->_lines[line].start = 0;
        break;

    case OLED_HALIGN_RIGHT:
        o->_lines[line].start = oled_screen_width - len;
        break;

    case OLED_HALIGN_CENTER:
        o->_lines[line].start = (oled_screen_width - len) >> 1;
        break;
    }

    return true;
}


static inline bool
oled_task(volatile oled_t *o)
{
    if (o == NULL || !o->_initialized)
        return false;

    if (o->_waiting) {
        if (o->_stop || (TWI0.MSTATUS & TWI_WIF_bm && !(TWI0.MSTATUS & TWI_RXACK_bm))) {
            o->_stop = false;
            if (o->_task_state == OLED_TASK_STATE_DATAN) {
                if (o->_current_data++ >= o->_lines[o->_current_page].start) {
                    if ((o->_current_data - o->_lines[o->_current_page].start) % (oled_font_width + 1) == 0) {
                        o->_current_char++;
                    }
                }
                if (o->_current_data < oled_screen_width) {
                    o->_waiting = false;
                    return true;
                }
            }
            o->_task_state++;
            if (o->_task_state == OLED_TASK_STATE_END) {
                o->_task_state = OLED_TASK_STATE_ADDR1;
                o->_lines[o->_current_page++].rendered = true;
                o->_current_data = 0;
                o->_current_char = 0;
                if (o->_current_page >= oled_lines)
                    o->_current_page = 0;
            }
            o->_waiting = false;
            return true;
        }
        else if(TWI0.MSTATUS & (TWI_BUSERR_bm | TWI_ARBLOST_bm)) {
            o->_waiting = false;
            return false;
        }
        return true;
    }

    switch (o->_task_state) {
    case OLED_TASK_STATE_ADDR1:
        if (o->_lines[o->_current_page].rendered) {
            o->_current_page++;
            if (o->_current_page >= oled_lines) {
                o->_current_page = 0;
            }
            return true;
        }

    // fall through
    case OLED_TASK_STATE_ADDR2:
        TWI0.MADDR = (oled_i2c_address << 1);
        o->_waiting = true;
        break;

    case OLED_TASK_STATE_STOP1:
    case OLED_TASK_STATE_STOP2:
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        o->_waiting = true;
        o->_stop = true;
        break;

    case OLED_TASK_STATE_RENDER1:
        // Co = 0; D/|C = 0
        TWI0.MDATA = 0x00;
        o->_waiting = true;
        break;

    case OLED_TASK_STATE_RENDER2:
        // set page address
        TWI0.MDATA = 0xB0 | o->_current_page;
        o->_waiting = true;
        break;

    case OLED_TASK_STATE_RENDER3:
        // set column address 4 lower bits (0 by default)
        TWI0.MDATA = 0x00;
        o->_waiting = true;
        break;

    case OLED_TASK_STATE_RENDER4:
        // set column address 4 higher bits (0)
        TWI0.MDATA = 0x10;
        o->_waiting = true;
        break;

    case OLED_TASK_STATE_DATA1:
        // Co = 0; D/|C = 1
        TWI0.MDATA = 0x40;
        o->_waiting = true;
        break;

    case OLED_TASK_STATE_DATAN:
        if (o->_current_data < o->_lines[o->_current_page].start) {
            TWI0.MDATA = 0;
        }
        else {
            char c = o->_lines[o->_current_page].data[o->_current_char];
            if (c == 0)
                o->_lines[o->_current_page].end = true;
            uint8_t i = (o->_current_data - o->_lines[o->_current_page].start) % (oled_font_width + 1);
            if (o->_lines[o->_current_page].end || i == oled_font_width)
                TWI0.MDATA = 0;
            else
                TWI0.MDATA = pgm_read_byte(&(oled_font[(uint8_t) c][i]));
        }
        o->_waiting = true;
        break;

    case OLED_TASK_STATE_END:
        break;
    }

    return true;
}
