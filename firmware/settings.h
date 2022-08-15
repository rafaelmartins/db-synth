/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// we could use EEMEM to store eeprom settings to .eeprom ELF section and export
// a separated hex file to initialize that, but managing everything in runtime
// is easier for users, and allow for settings versioning with migrations.

#define SETTINGS_VERSION 1

typedef struct __attribute__((packed)) {
    uint8_t _padding1;
    uint8_t version;
    uint8_t midi_channel;
    uint8_t _padding2[13];

    struct __attribute__((packed)) {
        uint8_t waveform;
        uint8_t _padding[15];
    } oscillator;

    struct __attribute__((packed)) {
        uint8_t attack;
        uint8_t decay;
        uint8_t sustain;
        uint8_t release;
        uint8_t type;  // not implemented yet
        uint8_t _padding[11];
    } adsr;

    struct __attribute__((packed)) {
        uint8_t type;
        uint8_t cutoff;
        uint8_t _padding[14];
    } filter;
} settings_data_t;

typedef struct {
    bool midi_channel;

    struct {
        bool waveform;
    } oscillator;

    struct {
        bool attack;
        bool decay;
        bool sustain;
        bool release;
    } adsr;

    struct {
        bool type;
        bool cutoff;
    } filter;
} settings_pending_t;

typedef struct {
    settings_data_t data;
    settings_pending_t pending;
    bool _initialized;
    bool _write;
} settings_t;


static inline bool
settings_init(settings_t *s, const settings_data_t *factory)
{
    if (s == NULL || s->_initialized)
        return false;

    memset(&s->pending, 0, sizeof(settings_pending_t));
    eeprom_read_block(&s->data, 0, sizeof(settings_data_t));

    if (s->data.version == 0xff) {  // not initialized
        // write factory settings
        memcpy_P(&s->data, factory, sizeof(settings_data_t));
        eeprom_write_block(&s->data, 0, sizeof(settings_data_t));
    }

    s->_initialized = true;

    return s->data.version == SETTINGS_VERSION;
}


static inline void
settings_start_write(settings_t *s)
{
    if (s != NULL && s->_initialized)
        s->_write = true;
}


static inline bool
settings_task(settings_t *s)
{
    if (s == NULL || !s->_initialized || !s->_write)
        return false;

#define _eeprom_addr(member) ((uint8_t*) (((uint8_t*) member) - ((uint8_t*) &s->data)))

    // process only the first pending and return

    if (s->pending.midi_channel) {
        eeprom_write_byte(_eeprom_addr(&s->data.midi_channel), s->data.midi_channel);
        s->pending.midi_channel = false;
        return true;
    }
    if (s->pending.oscillator.waveform) {
        eeprom_write_byte(_eeprom_addr(&s->data.oscillator.waveform), s->data.oscillator.waveform);
        s->pending.oscillator.waveform = false;
        return true;
    }
    if (s->pending.adsr.attack) {
        eeprom_write_byte(_eeprom_addr(&s->data.adsr.attack), s->data.adsr.attack);
        s->pending.adsr.attack = false;
        return true;
    }
    if (s->pending.adsr.decay) {
        eeprom_write_byte(_eeprom_addr(&s->data.adsr.decay), s->data.adsr.decay);
        s->pending.adsr.decay = false;
        return true;
    }
    if (s->pending.adsr.sustain) {
        eeprom_write_byte(_eeprom_addr(&s->data.adsr.sustain), s->data.adsr.sustain);
        s->pending.adsr.sustain = false;
        return true;
    }
    if (s->pending.adsr.release) {
        eeprom_write_byte(_eeprom_addr(&s->data.adsr.release), s->data.adsr.release);
        s->pending.adsr.release = false;
        return true;
    }
    if (s->pending.filter.type) {
        eeprom_write_byte(_eeprom_addr(&s->data.filter.type), s->data.filter.type);
        s->pending.filter.type = false;
        return true;
    }
    if (s->pending.filter.cutoff) {
        eeprom_write_byte(_eeprom_addr(&s->data.filter.cutoff), s->data.filter.cutoff);
        s->pending.filter.cutoff = false;
        return true;
    }

#undef _eeprom_addr

    s->_write = false;
    return false;
}
