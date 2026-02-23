/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "settings.h"


bool
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


void
settings_start_write(settings_t *s)
{
    if (s != NULL && s->_initialized)
        s->_write = true;
}


bool
settings_task(settings_t *s)
{
    if (s == NULL || !s->_initialized || !s->_write)
        return false;

#define _eeprom_addr(member) ((uint8_t*) (((uint8_t*) member) - ((uint8_t*) &s->data)))

    // process only the first pending and return

    if (s->pending.midi_channel) {
        eeprom_write_byte(_eeprom_addr(&s->data.midi_channel), s->data.midi_channel);
        s->pending.midi_channel = false;
        return false;
    }
    if (s->pending.oscillator.waveform) {
        eeprom_write_byte(_eeprom_addr(&s->data.oscillator.waveform), s->data.oscillator.waveform);
        s->pending.oscillator.waveform = false;
        return false;
    }
    if (s->pending.adsr.type) {
        eeprom_write_byte(_eeprom_addr(&s->data.adsr.type), s->data.adsr.type);
        s->pending.adsr.type = false;
        return false;
    }
    if (s->pending.adsr.attack) {
        eeprom_write_byte(_eeprom_addr(&s->data.adsr.attack), s->data.adsr.attack);
        s->pending.adsr.attack = false;
        return false;
    }
    if (s->pending.adsr.decay) {
        eeprom_write_byte(_eeprom_addr(&s->data.adsr.decay), s->data.adsr.decay);
        s->pending.adsr.decay = false;
        return false;
    }
    if (s->pending.adsr.sustain) {
        eeprom_write_byte(_eeprom_addr(&s->data.adsr.sustain), s->data.adsr.sustain);
        s->pending.adsr.sustain = false;
        return false;
    }
    if (s->pending.adsr.release) {
        eeprom_write_byte(_eeprom_addr(&s->data.adsr.release), s->data.adsr.release);
        s->pending.adsr.release = false;
        return false;
    }
    if (s->pending.filter.type) {
        eeprom_write_byte(_eeprom_addr(&s->data.filter.type), s->data.filter.type);
        s->pending.filter.type = false;
        return false;
    }
    if (s->pending.filter.cutoff) {
        eeprom_write_byte(_eeprom_addr(&s->data.filter.cutoff), s->data.filter.cutoff);
        s->pending.filter.cutoff = false;
        return false;
    }

#undef _eeprom_addr

    s->_write = false;
    return true;
}
