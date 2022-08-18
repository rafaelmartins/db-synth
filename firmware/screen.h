/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "filter.h"
#include "oled.h"
#include "oscillator.h"
#include "screen-data.h"

// there's just one screen for everything:

// ......db-synth.......
// .....................
// WF: Triangle | CH: 15
// .....................
// A: 20.00s | D: 20.00s
// S: 100.0% | R: 20.00s
// .....................
// F: LPF | FC: 20.00kHz

typedef struct {
    oled_t oled;
    bool _initialized;
    bool _notification;
    uint16_t _notification_count;
    char _line2[oled_chars_per_line + 1];
    char _line4[oled_chars_per_line + 1];
    char _line5[oled_chars_per_line + 1];
    char _line7[oled_chars_per_line + 1];
} screen_t;


static inline bool
screen_init(screen_t *s)
{
    if (s == NULL || s->_initialized)
        return false;

    if (!oled_init(&s->oled))
        return false;

    s->_initialized = true;

    strcpy(s->_line2, "WF:          | CH:   ");
    strcpy(s->_line4, "A:        | D:       ");
    strcpy(s->_line5, "S:        | R:       ");
    strcpy(s->_line7, "F:     | FC:         ");

    return oled_line(&s->oled, 0, "db-synth", OLED_HALIGN_CENTER);
}


static inline bool
screen_task(screen_t *s)
{
    if (s == NULL || !s->_initialized)
        return false;

    return oled_task(&s->oled);
}


static inline bool
screen_set_oscillator_waveform(screen_t *s, oscillator_waveform_t wf)
{
    if (s == NULL)
        return false;

    char *w;
    switch (wf) {
    case OSCILLATOR_WAVEFORM_SQUARE:
        w = "Square  ";
        break;
    case OSCILLATOR_WAVEFORM_SINE:
        w = "Sine    ";
        break;
    case OSCILLATOR_WAVEFORM_TRIANGLE:
        w = "Triangle";
        break;
    case OSCILLATOR_WAVEFORM_SAW:
        w = "Saw     ";
        break;
    default:
        w = "Unknown ";
        break;
    }

    memcpy(s->_line2 + 4, w, 8);
    return oled_line(&s->oled, 2, s->_line2, OLED_HALIGN_LEFT);
}


static inline bool
screen_set_midi_channel(screen_t *s, uint8_t midi_ch)
{
    if (s == NULL || midi_ch > 0xf)
        return false;

    midi_ch++;
    s->_line2[19] = midi_ch > 9 ? '1' : midi_ch + '0';
    s->_line2[20] = midi_ch > 9 ? (midi_ch % 10) + '0' : ' ';

    return oled_line(&s->oled, 2, s->_line2, OLED_HALIGN_LEFT);
}


static inline bool
screen_set_adsr_attack(screen_t *s, uint8_t v)
{
    if (s == NULL)
        return false;

    memcpy_P(s->_line4 + 3, adsr_time_descriptions[v], adsr_time_description_strlen);
    return oled_line(&s->oled, 4, s->_line4, OLED_HALIGN_LEFT);
}


static inline bool
screen_set_adsr_decay(screen_t *s, uint8_t v)
{
    if (s == NULL)
        return false;

    memcpy_P(s->_line4 + 15, adsr_time_descriptions[v], adsr_time_description_strlen);
    return oled_line(&s->oled, 4, s->_line4, OLED_HALIGN_LEFT);
}


static inline bool
screen_set_adsr_sustain(screen_t *s, uint8_t v)
{
    if (s == NULL)
        return false;

    memcpy_P(s->_line5 + 3, adsr_level_descriptions[v], adsr_level_description_strlen);
    return oled_line(&s->oled, 5, s->_line5, OLED_HALIGN_LEFT);
}


static inline bool
screen_set_adsr_release(screen_t *s, uint8_t v)
{
    if (s == NULL)
        return false;

    memcpy_P(s->_line5 + 15, adsr_time_descriptions[v], adsr_time_description_strlen);
    return oled_line(&s->oled, 5, s->_line5, OLED_HALIGN_LEFT);
}


static inline bool
screen_set_filter_type(screen_t *s, filter_type_t ft)
{
    if (s == NULL)
        return false;

    char *f;
    switch (ft) {
    case FILTER_TYPE_OFF:
        f = "Off";
        break;
    case FILTER_TYPE_LOW_PASS:
        f = "LPF";
        break;
    case FILTER_TYPE_HIGH_PASS:
        f = "HPF";
        break;
    default:
        f = "Unk";
        break;
    }

    memcpy(s->_line7 + 3, f, 3);
    return oled_line(&s->oled, 7, s->_line7, OLED_HALIGN_LEFT);
}


static inline bool
screen_set_filter_cutoff(screen_t *s, uint8_t c)
{
    if (s == NULL)
        return false;

    memcpy_P(s->_line7 + 13, filter_cutoff_descriptions[c], filter_cutoff_description_strlen);
    return oled_line(&s->oled, 7, s->_line7, OLED_HALIGN_LEFT);
}
