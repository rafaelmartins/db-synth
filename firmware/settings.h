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
        uint8_t type;
        uint8_t _padding[11];
    } adsr;

    // placeholder for filter settings, we may want to reimplement.
    uint8_t _filter[16];
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
        bool type;
    } adsr;
} settings_pending_t;

typedef struct {
    settings_data_t data;
    settings_pending_t pending;
    bool _initialized;
    bool _write;
} settings_t;

bool settings_init(settings_t *s, const settings_data_t *factory);
void settings_start_write(settings_t *s);
bool settings_task(settings_t *s);
