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
#include "adsr.h"
#include "filter.h"
#include "oled.h"
#include "oscillator.h"
#include "screen-data.h"

// there's just one screen for everything:

// [db-synth] PRESET UPD
// .....................
// WF: Triangle | CH: 15
// .....................
// AE: 20.0s | DE: 20.0s
// S: 100.0% | RE: 20.0s
// .....................
// F: LPF | FC: 20.00kHz

typedef struct {
    oled_t oled;
    bool _initialized;
    bool _notification;
    uint16_t _notification_count;
    uint8_t _notification_sec;
    char _line0[oled_chars_per_line + 1];
    char _line2[oled_chars_per_line + 1];
    char _line4[oled_chars_per_line + 1];
    char _line5[oled_chars_per_line + 1];
    char _line7[oled_chars_per_line + 1];
} screen_t;

typedef enum {
    SCREEN_NOTIFICATION_ERROR,  // not used yet
    SCREEN_NOTIFICATION_READY,
    SCREEN_NOTIFICATION_PRESET_UPDATED,
} screen_notification_t;

bool screen_init(screen_t *s);
bool screen_task(screen_t *s);
bool screen_notification(screen_t *s, screen_notification_t notif);
bool screen_set_oscillator_waveform(screen_t *s, oscillator_waveform_t wf);
bool screen_set_midi_channel(screen_t *s, uint8_t midi_ch);
bool screen_set_adsr_type(screen_t *s, adsr_type_t t);
bool screen_set_adsr_attack(screen_t *s, uint8_t v);
bool screen_set_adsr_decay(screen_t *s, uint8_t v);
bool screen_set_adsr_sustain(screen_t *s, uint8_t v);
bool screen_set_adsr_release(screen_t *s, uint8_t v);
bool screen_set_filter_type(screen_t *s, filter_type_t ft);
bool screen_set_filter_cutoff(screen_t *s, uint8_t c);
