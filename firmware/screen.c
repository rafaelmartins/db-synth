/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "adsr.h"
#include "oled.h"
#include "oscillator.h"
#include "screen.h"
#include "screen-data.h"

// there's just one screen for all synthesizer settings data:

// [db-synth] PRESET UPD
// .....................
// WF: Triangle | CH: 15
// .....................
// AE: 20.0s | DE: 20.0s
// S: 100.0% | RE: 20.0s
// .....................
// .....................


// a notification screen appears for a few seconds (but the context of
// the main screen is still kept in sync with data in background):

// .....................
// .......header........
// .....................
// .......message.......
// .....................
// .....................
// .......footer........
// .....................


bool
screen_init(screen_t *s)
{
    if (s == NULL || s->_initialized)
        return false;

    if (!oled_init(&s->oled))
        return false;

    s->_initialized = true;
    s->_notification = false;

    memcpy(s->_line0, "[db-synth]           ", 22);
    memcpy(s->_line2, "WF:          | CH:   ", 22);
    memcpy(s->_line4, "A :       | D :      ", 22);
    memcpy(s->_line5, "S:        | R :      ", 22);

    return screen_notification(s, SCREEN_NOTIFICATION_SPLASH);
}


bool
screen_task(screen_t *s)
{
    if (s == NULL || !s->_initialized)
        return false;

    // TODO: we have a spare TCB timer available, maybe use it for this?
    if (s->_notification && ++s->_notification_count == sample_rate && ++s->_notification_sec == 2) {
        oled_line(&s->oled, 0, s->_line0, OLED_HALIGN_LEFT);
        oled_line(&s->oled, 1, "", OLED_HALIGN_LEFT);
        oled_line(&s->oled, 2, s->_line2, OLED_HALIGN_LEFT);
        oled_line(&s->oled, 3, "", OLED_HALIGN_LEFT);
        oled_line(&s->oled, 4, s->_line4, OLED_HALIGN_LEFT);
        oled_line(&s->oled, 5, s->_line5, OLED_HALIGN_LEFT);
        oled_line(&s->oled, 6, "", OLED_HALIGN_LEFT);
        oled_line(&s->oled, 7, "", OLED_HALIGN_LEFT);
        s->_notification = false;
    }

    return oled_task(&s->oled);
}


bool
screen_notification(screen_t *s, screen_notification_t n)
{
    if (s == NULL)
        return false;

    s->_notification_count = 0;
    s->_notification_sec = 0;
    s->_notification = true;

    char *h = "NOTIFICATION";
    char *m = NULL;
    char *f = NULL;
    switch (n) {
    case SCREEN_NOTIFICATION_SPLASH:
        h = "[db-synth]";
        m = DB_SYNTH_VERSION;
        f = "db-synth.rgm.io";
        break;
    case SCREEN_NOTIFICATION_PRESET_UPDATED:
        m = "PRESET UPDATED";
        break;
    default:
        return false;
    }

    oled_line(&s->oled, 0, "", OLED_HALIGN_LEFT);
    oled_line(&s->oled, 1, h == NULL ? "" : h, OLED_HALIGN_CENTER);
    oled_line(&s->oled, 2, "", OLED_HALIGN_LEFT);
    oled_line(&s->oled, 3, m == NULL ? "" : m, OLED_HALIGN_CENTER);
    oled_line(&s->oled, 4, "", OLED_HALIGN_LEFT);
    oled_line(&s->oled, 5, "", OLED_HALIGN_LEFT);
    oled_line(&s->oled, 6, f == NULL ? "" : f, OLED_HALIGN_CENTER);
    oled_line(&s->oled, 7, "", OLED_HALIGN_LEFT);

    // FIXME: chack return of previous oled_line calls
    return true;
}


bool
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
    if (s->_notification)
        return true;
    return oled_line(&s->oled, 2, s->_line2, OLED_HALIGN_LEFT);
}


bool
screen_set_midi_channel(screen_t *s, uint8_t midi_ch)
{
    if (s == NULL || midi_ch > 0xf)
        return false;

    midi_ch++;
    s->_line2[19] = midi_ch > 9 ? '1' : midi_ch + '0';
    s->_line2[20] = midi_ch > 9 ? (midi_ch % 10) + '0' : ' ';

    if (s->_notification)
        return true;
    return oled_line(&s->oled, 2, s->_line2, OLED_HALIGN_LEFT);
}


bool
screen_set_adsr_type(screen_t *s, adsr_type_t t)
{
    if (s == NULL)
        return false;

    s->_line4[1] = s->_line4[13] = s->_line5[13] = t == ADSR_TYPE_LINEAR ? 'L' : 'E';

    if (s->_notification)
        return true;
    return oled_line(&s->oled, 4, s->_line4, OLED_HALIGN_LEFT) && oled_line(&s->oled, 5, s->_line5, OLED_HALIGN_LEFT);
}


bool
screen_set_adsr_attack(screen_t *s, uint8_t v)
{
    if (s == NULL || v >= adsr_time_descriptions_rows)
        return false;

    memcpy(s->_line4 + 4, adsr_time_descriptions[v], adsr_time_descriptions_cols);

    if (s->_notification)
        return true;
    return oled_line(&s->oled, 4, s->_line4, OLED_HALIGN_LEFT);
}


bool
screen_set_adsr_decay(screen_t *s, uint8_t v)
{
    if (s == NULL || v >= adsr_time_descriptions_rows)
        return false;

    memcpy(s->_line4 + 16, adsr_time_descriptions[v], adsr_time_descriptions_cols);

    if (s->_notification)
        return true;
    return oled_line(&s->oled, 4, s->_line4, OLED_HALIGN_LEFT);
}


bool
screen_set_adsr_sustain(screen_t *s, uint8_t v)
{
    if (s == NULL || v >= adsr_level_descriptions_rows)
        return false;

    memcpy(s->_line5 + 3, adsr_level_descriptions[v], adsr_level_descriptions_cols);

    if (s->_notification)
        return true;
    return oled_line(&s->oled, 5, s->_line5, OLED_HALIGN_LEFT);
}


bool
screen_set_adsr_release(screen_t *s, uint8_t v)
{
    if (s == NULL || v >= adsr_time_descriptions_rows)
        return false;

    memcpy(s->_line5 + 16, adsr_time_descriptions[v], adsr_time_descriptions_cols);

    if (s->_notification)
        return true;
    return oled_line(&s->oled, 5, s->_line5, OLED_HALIGN_LEFT);
}
