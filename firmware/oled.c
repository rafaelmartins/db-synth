/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include "oled.h"

#define I2C_ADDRESS 0x3c
#define FONT_WIDTH 5
#define FONT_HEIGHT 7
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define MAX_CHARS_PER_LINE ((uint8_t) (SCREEN_WIDTH / (FONT_WIDTH + 1)))
#define RAM_PAGES          ((uint8_t) (SCREEN_HEIGHT / (FONT_HEIGHT + 1)))
// we abuse the fact that our rendered lines match nicely with the ram pages
#define MAX_LINES RAM_PAGES

extern const uint8_t font_bitmaps[];
extern const uint16_t font_descriptors[];

static volatile struct {
    uint8_t data[128];
    bool rendered;
} ram_pages[8];

static volatile enum {
    STATE_ADDR1,
    STATE_RENDER1,
    STATE_RENDER2,
    STATE_RENDER3,
    STATE_RENDER4,
    STATE_STOP1,
    STATE_ADDR2,
    STATE_DATA1,
    STATE_DATAN,
    STATE_STOP2,
    STATE_END,
} task_state = STATE_ADDR1;

static volatile bool initialized = false;
static volatile bool waiting = false;
static volatile bool stop = false;
static volatile uint8_t current_page = 0;
static volatile uint8_t current_data = 0;


bool
oled_clear_line(uint8_t line)
{
    if (line >= MAX_LINES)
        return false;

    memset((void*) ram_pages[line].data + 1, 0, sizeof(ram_pages[line].data) - 1);
    ram_pages[line].rendered = false;

    return true;
}


bool
oled_clear(void)
{
    for (uint8_t i = 0; i < RAM_PAGES; i++) {
        if (!oled_clear_line(i)) {
            return false;
        }
    }
    return true;
}


static bool
twi_wait(void)
{
    while (true) {
        if (TWI0.MSTATUS & TWI_WIF_bm)
            return !(TWI0.MSTATUS & TWI_RXACK_bm);
        else if(TWI0.MSTATUS & (TWI_BUSERR_bm | TWI_ARBLOST_bm))
            return false;
    }
    return false;
}


bool
oled_init(void)
{
    PORTMUX.TWIROUTEA = PORTMUX_TWI0_ALT2_gc;
    TWI0.CTRLA = TWI_INPUTLVL_I2C_gc | TWI_SDASETUP_4CYC_gc | TWI_SDAHOLD_OFF_gc | TWI_FMPEN_OFF_gc;
    TWI0.MBAUD = 25;  // 400khz (rise time 0)
    TWI0.MCTRLA = TWI_ENABLE_bm;
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;

    TWI0.MADDR = (I2C_ADDRESS << 1);
    if (!twi_wait())
        return false;

    // Co = 0; D/|C = 0
    TWI0.MDATA = 0x00;
    if (!twi_wait())
        return false;

    // set segment remap (reverse direction)
    TWI0.MDATA = 0xA1;
    if (!twi_wait())
        return false;

    // set COM output scan direction (COM[N-1] to COM0)
    TWI0.MDATA = 0xC8;
    if (!twi_wait())
        return false;

    // charge pump setting (enable during display on)
    TWI0.MDATA = 0x8D;
    if (!twi_wait())
        return false;
    TWI0.MDATA = 0x14;
    if (!twi_wait())
        return false;

    // set COM output scan direction (COM[N-1] to COM0)
    TWI0.MDATA = 0xAF;
    if (!twi_wait())
        return false;

    TWI0.MCTRLB = TWI_MCMD_STOP_gc;

    initialized = true;

    return oled_clear();
}


bool
oled_line(uint8_t line, const char *str, oled_halign_t align)
{
    if (str == NULL)
        return false;

    if (line >= MAX_LINES)
        return false;

    uint8_t slen = 0;
    for (uint8_t i = 0; str[i] != '\0' && i < MAX_CHARS_PER_LINE;) {
        slen += FONT_WIDTH;
        if (str[++i] != '\0')
            slen++;
    }

    if (slen > SCREEN_WIDTH)
        return false;

    uint8_t x = 0;
    switch (align) {
        case OLED_HALIGN_LEFT:
            break;
        case OLED_HALIGN_RIGHT:
            x = SCREEN_WIDTH - slen;
            break;
        case OLED_HALIGN_CENTER:
            x = (SCREEN_WIDTH - slen) / 2;
            break;
    }

    memset((void*) ram_pages[line].data, 0, sizeof(ram_pages[line].data));

    for (uint8_t i = 0; str[i] != '\0' && i < MAX_CHARS_PER_LINE;) {
        uint16_t offset = *(font_descriptors + str[i]);
        const uint8_t *bm = font_bitmaps + offset;

        for (uint8_t j = 0; j < FONT_HEIGHT; ++j) {
            for (uint8_t k = 0; k < FONT_WIDTH; ++k) {
                if ((bm[j] << k) & (1 << 7))
                    ram_pages[line].data[x + k] |= (1 << j);
                else
                    ram_pages[line].data[x + k] &= ~(1 << j);
            }
        }

        x += FONT_WIDTH;
        if (str[++i] != '\0')
            x++;
    }

    ram_pages[line].rendered = false;

    return true;
}


bool
oled_task(void)
{
    if (!initialized)
        return true;

    if (waiting) {
        if (stop || (TWI0.MSTATUS & TWI_WIF_bm && !(TWI0.MSTATUS & TWI_RXACK_bm))) {
            stop = false;
            if (task_state == STATE_DATAN) {
                if (current_data < sizeof(ram_pages[current_page].data)) {
                    waiting = false;
                    return true;
                }
            }
            task_state++;
            if (task_state == STATE_END) {
                task_state = STATE_ADDR1;
                ram_pages[current_page++].rendered = true;
                current_data = 0;
                if (current_page >= RAM_PAGES)
                    current_page = 0;
            }
            waiting = false;
            return true;
        }
        else if(TWI0.MSTATUS & (TWI_BUSERR_bm | TWI_ARBLOST_bm)) {
            waiting = false;
            return false;
        }
        return true;
    }

    switch (task_state) {
    case STATE_ADDR1:
        if (ram_pages[current_page].rendered) {
            current_page++;
            if (current_page >= RAM_PAGES) {
                current_page = 0;
            }
            return true;
        }

    // fall through
    case STATE_ADDR2:
        TWI0.MADDR = (I2C_ADDRESS << 1);
        waiting = true;
        break;

    case STATE_STOP1:
    case STATE_STOP2:
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        waiting = true;
        stop = true;
        break;

    case STATE_RENDER1:
        // Co = 0; D/|C = 0
        TWI0.MDATA = 0x00;
        waiting = true;
        break;

    case STATE_RENDER2:
        // set page address
        TWI0.MDATA = 0xB0 | current_page;
        waiting = true;
        break;

    case STATE_RENDER3:
        // set column address 4 lower bits (0 by default)
        TWI0.MDATA = 0x00;
        waiting = true;
        break;

    case STATE_RENDER4:
        // set column address 4 higher bits (0)
        TWI0.MDATA = 0x10;
        waiting = true;
        break;

    case STATE_DATA1:
        // Co = 0; D/|C = 1
        TWI0.MDATA = 0x40;
        waiting = true;
        break;

    case STATE_DATAN:
        TWI0.MDATA = ram_pages[current_page].data[current_data++];
        waiting = true;
        break;

    case STATE_END:
        break;
    }

    return true;
}
