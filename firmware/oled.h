/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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

typedef enum {
    _TWI_WAITING,
    _TWI_ACK,
    _TWI_ERROR,
} _twi_state_t;


static inline _twi_state_t
_twi_check_state(void)
{
    uint8_t status = TWI0.MSTATUS;
    if (status & TWI_WIF_bm) {
        if (status & (TWI_BUSERR_bm | TWI_ARBLOST_bm))
            return _TWI_ERROR;
        return (status & TWI_RXACK_bm) ? _TWI_ERROR : _TWI_ACK;
    }
    return _TWI_WAITING;
}


static inline bool
_twi_wait(void)
{
    while (true) {
        switch (_twi_check_state()) {
        case _TWI_WAITING: break;
        case _TWI_ACK:     return true;
        case _TWI_ERROR:   return false;
        }
    }
    return false;
}


static inline bool
oled_init(oled_t *o)
{
    if (o == NULL || o->_initialized)
        return false;

    // the cheap SSD1306 oled breakouts we use here already have pull-up resistors.
    // the internal ones are weak, have the same power supply (which means they are
    // in parallel), and seem to help with the stability. enable the internal ones too.
    PORTA.PIN2CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN3CTRL = PORT_PULLUPEN_bm;

    // there's an errata for output state of twi pins, affecting every chip produced.
    // clear them out before enabling twi.
    PORTA.OUTCLR = PIN2_bm | PIN3_bm;

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

    // display on
    TWI0.MDATA = 0xAF;
    if (!_twi_wait())
        return false;

    TWI0.MCTRLB = TWI_MCMD_STOP_gc;

    // first screen cleanup is synchronous
    for (uint8_t i = 0; i < oled_lines; i++) {
        TWI0.MADDR = (oled_i2c_address << 1);
        if (!_twi_wait())
            return false;

        // Co = 0; D/|C = 0
        TWI0.MDATA = 0x00;
        if (!_twi_wait())
            return false;

        // set line address
        TWI0.MDATA = 0xB0 | i;
        if (!_twi_wait())
            return false;

        // set column address 4 lower bits
        TWI0.MDATA = 0x0f;
        if (!_twi_wait())
            return false;

        // set column address 4 higher bits
        TWI0.MDATA = 0x10;
        if (!_twi_wait())
            return false;

        TWI0.MCTRLB = TWI_MCMD_STOP_gc;

        TWI0.MADDR = (oled_i2c_address << 1);
        if (!_twi_wait())
            return false;

        // Co = 0; D/|C = 1
        TWI0.MDATA = 0x40;
        if (!_twi_wait())
            return false;

        for (uint8_t j = 0; j < oled_screen_width; j++) {
            TWI0.MDATA = 0x00;
            if (!_twi_wait())
                return false;
        }

        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
    }

    o->_polling = false;
    o->_initialized = true;

    return true;
}


static inline bool
oled_line(oled_t *o, uint8_t line, const char *str, oled_halign_t align)
{
    if (o == NULL || !o->_initialized)
        return false;

    if (str == NULL || line >= oled_lines || o->_lines[line].state == LINE_STATE_RENDERING)
        return false;

    uint8_t start = 0;
    uint8_t len = strlen(str);

    switch (align) {
    case OLED_HALIGN_LEFT:
        break;

    case OLED_HALIGN_RIGHT:
        start = oled_chars_per_line - len;
        break;

    case OLED_HALIGN_CENTER:
        start = (oled_chars_per_line - len) >> 1;
        break;
    }

    memset(o->_lines[line].data, ' ', sizeof(o->_lines[line].data));

    const char *c = str;
    uint8_t i = start;
    while (i < oled_chars_per_line && *c != 0)
        o->_lines[line].data[i++] = *c++;

    o->_lines[line].state = LINE_STATE_PENDING;

    return true;
}


static inline void
_oled_task_send(oled_t *o)
{
    // the state of the oled_t pointer is checked by the caller.

    switch (o->_task_state) {
    case OLED_TASK_STATE_ADDR1:
        if (o->_lines[o->_current_line].state != LINE_STATE_PENDING)
            break;

    // fall through
    case OLED_TASK_STATE_ADDR2:
        o->_lines[o->_current_line].state = LINE_STATE_RENDERING;
        TWI0.MADDR = (oled_i2c_address << 1);
        break;

    case OLED_TASK_STATE_STOP1:
    case OLED_TASK_STATE_STOP2:
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        break;

    case OLED_TASK_STATE_RENDER1:
        // Co = 0; D/|C = 0
        TWI0.MDATA = 0x00;
        break;

    case OLED_TASK_STATE_RENDER2:
        // set line address
        TWI0.MDATA = 0xB0 | o->_current_line;
        break;

    case OLED_TASK_STATE_RENDER3:
        // set column address 4 lower bits
        // there are 2 pixels remaining from our printable area, we just push them left
        TWI0.MDATA = 0x00 | 0x02;
        break;

    case OLED_TASK_STATE_RENDER4:
        // set column address 4 higher bits
        TWI0.MDATA = 0x10;
        break;

    case OLED_TASK_STATE_DATA1:
        // Co = 0; D/|C = 1
        TWI0.MDATA = 0x40;
        break;

    case OLED_TASK_STATE_DATAN: {
        char c = o->_lines[o->_current_line].data[o->_current_char];
        uint8_t i = o->_current_data % (oled_font_width + 1);
        if (i == oled_font_width)
            TWI0.MDATA = 0;
        else
            TWI0.MDATA = pgm_read_byte(&(oled_font[(uint8_t) c][i]));
        break;
    }

    case OLED_TASK_STATE_END:
        o->_lines[o->_current_line].state = LINE_STATE_FREE;
        break;
    }
}


static inline _twi_state_t
_oled_task_poll(oled_t *o)
{
    // the state of the oled_t pointer is checked by the caller.

    _twi_state_t rv = _TWI_ACK;

    switch (o->_task_state) {
    case OLED_TASK_STATE_ADDR1:
        if (o->_lines[o->_current_line].state != LINE_STATE_RENDERING) {
            o->_current_line++;
            if (o->_current_line == oled_lines)
                o->_current_line = 0;
            return rv;
        }

    // fall through
    case OLED_TASK_STATE_RENDER1:
    case OLED_TASK_STATE_RENDER2:
    case OLED_TASK_STATE_RENDER3:
    case OLED_TASK_STATE_RENDER4:
    case OLED_TASK_STATE_ADDR2:
        rv = _twi_check_state();
        if (rv != _TWI_ACK)
            return rv;
        o->_task_state++;
        return rv;

    case OLED_TASK_STATE_DATA1:
        rv = _twi_check_state();
        if (rv != _TWI_ACK)
            return rv;
        o->_task_state++;
        o->_current_char = 0;
        o->_current_data = 0;
        return rv;

    case OLED_TASK_STATE_DATAN:
        rv = _twi_check_state();
        if (rv != _TWI_ACK)
            return rv;
        if (++o->_current_data == oled_screen_width) {
            o->_task_state++;
            return rv;
        }
        if ((o->_current_data % (oled_font_width + 1)) == 0)
            o->_current_char++;
        return rv;

    case OLED_TASK_STATE_STOP1:
    case OLED_TASK_STATE_STOP2:
        o->_task_state++;
        return rv;

    case OLED_TASK_STATE_END:
        o->_task_state = OLED_TASK_STATE_ADDR1;
        return rv;
    }

    return true;
}


static inline bool
oled_task(oled_t *o)
{
    if (o == NULL || !o->_initialized)
        return false;

    if (o->_polling) {
        switch(_oled_task_poll(o)) {
        case _TWI_ACK:
            o->_polling = false;
            break;
        case _TWI_WAITING:
            break;
        case _TWI_ERROR:
            return false;
        }
        return true;
    }

    _oled_task_send(o);
    o->_polling = true;
    return true;
}
