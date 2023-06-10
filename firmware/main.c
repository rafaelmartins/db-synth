/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include "adsr.h"
#include "amplifier.h"
#include "midi.h"
#include "oscillator.h"
#include "screen.h"
#include "settings.h"
#include "main-data.h"

FUSES =
{
    .WDTCFG = WINDOW_OFF_gc | PERIOD_OFF_gc,
    .BODCFG = LVL_BODLEVEL3_gc | SAMPFREQ_128Hz_gc | ACTIVE_SAMPLE_gc | SLEEP_SAMPLE_gc,
    .OSCCFG = CLKSEL_OSCHF_gc,
    .SYSCFG0 = CRCSRC_NOCRC_gc | CRCSEL_CRC16_gc | RSTPINCFG_GPIO_gc | FUSE_EESAVE_bm,
    .SYSCFG1 = MVSYSCFG_SINGLE_gc | SUT_0MS_gc,
    .CODESIZE = 0,
    .BOOTSIZE = 0,
};

static adsr_t adsr;
static midi_t midi;
static oscillator_t oscillator;
static screen_t screen;
static settings_t settings;
static uint8_t note;
static uint8_t velocity;

static const settings_data_t factory_settings PROGMEM = {
    .version = SETTINGS_VERSION,
    .midi_channel = 0,
    .oscillator = {
        .waveform = OSCILLATOR_WAVEFORM_SQUARE,
    },
    .adsr = {
        .type = ADSR_TYPE_EXPONENTIAL,
        .attack = 0x08,
        .decay = 0x08,
        .sustain = 0x60,
        .release = 0x08,
    },
};


static inline void
clock_init(void)
{
    // setup internal high frequency clock
    _PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, CLKCTRL_RUNSTDBY_bm | cpu_frqsel | CLKCTRL_AUTOTUNE_bm);
    while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_OSCHFS_bm));

    // use internal high frequency clock as main clock
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSCHF_gc);
    while (CLKCTRL.MCLKSTATUS & CLKCTRL_SOSC_bm);
}


static inline void
opamp_init(void)
{
    // disable input for pins used by opamps
    PORTD.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTD.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTD.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;

    // 1us @ CLK_PER, minus 1
    OPAMP.TIMEBASE = opamp_timebase;

    // OPAMP 0: unity gain for DAC
    OPAMP.OP0INMUX = OPAMP_OP0INMUX_MUXNEG_OUT_gc | OPAMP_OP0INMUX_MUXPOS_DAC_gc;
    OPAMP.OP0CTRLA = OPAMP_RUNSTBY_bm | OPAMP_OP0CTRLA_OUTMODE_NORMAL_gc | OPAMP_ALWAYSON_bm;

    // OPAMP 1: 2nd order low pass filter
    OPAMP.OP1INMUX = OPAMP_OP1INMUX_MUXNEG_WIP_gc | OPAMP_OP1INMUX_MUXPOS_INP_gc;
    OPAMP.OP1RESMUX = OPAMP_OP1RESMUX_MUXWIP_WIP2_gc | OPAMP_OP1RESMUX_MUXBOT_GND_gc | OPAMP_OP1RESMUX_MUXTOP_OUT_gc;
    OPAMP.OP1CTRLA = OPAMP_RUNSTBY_bm | OPAMP_OP1CTRLA_OUTMODE_NORMAL_gc | OPAMP_ALWAYSON_bm;

    // enable opamps
    OPAMP.CTRLA = OPAMP_ENABLE_bm;
}


static inline void
dac_init(void)
{
    // DAC Vref = 2.5V
    VREF.DAC0REF = VREF_ALWAYSON_bm | VREF_REFSEL_2V500_gc;

    // enable DAC and output buffer (required by opamp)
    DAC0.CTRLA = DAC_RUNSTDBY_bm | DAC_ENABLE_bm | DAC_OUTEN_bm;
}


static inline void
midi_channel_cb(midi_command_t cmd, uint8_t ch, uint8_t *buf, uint8_t len)
{
    if (ch != settings.data.midi_channel && !(cmd == MIDI_CONTROL_CHANGE && buf[0] == 0x66))
        return;

    switch (cmd) {
    case MIDI_NOTE_ON:
        if (len == 2 && buf[0] != 0) {
            oscillator_set_note(&oscillator, buf[0]);
            note = buf[0];
            velocity = buf[1] * 2;
            adsr_set_gate(&adsr);
            break;
        }

    // fall through
    case MIDI_NOTE_OFF:
        if (buf[0] == note)
            adsr_unset_gate(&adsr, false);
        break;

    case MIDI_CONTROL_CHANGE:
        if (len != 2)
            break;

        switch (buf[0]) {
        case 3:  // waveform
            settings.data.oscillator.waveform = buf[1] / (0x80 / OSCILLATOR_WAVEFORM__LAST);
            if (settings.data.oscillator.waveform >= OSCILLATOR_WAVEFORM__LAST)
                settings.data.oscillator.waveform--;
            settings.pending.oscillator.waveform = true;
            if (oscillator_set_waveform(&oscillator, settings.data.oscillator.waveform))
                screen_set_oscillator_waveform(&screen, settings.data.oscillator.waveform);
            break;

        case 70:  // adsr type
            settings.data.adsr.type = buf[1] / (0x80 / ADSR_TYPE__LAST);
            if (settings.data.adsr.type >= ADSR_TYPE__LAST)
                settings.data.adsr.type--;
            settings.pending.adsr.type = true;
            if (adsr_set_type(&adsr, settings.data.adsr.type))
                screen_set_adsr_type(&screen, settings.data.adsr.type);
            break;

        case 72:  // adsr release
            settings.data.adsr.release = buf[1];
            settings.pending.adsr.release = true;
            if (adsr_set_release(&adsr, settings.data.adsr.release))
                screen_set_adsr_release(&screen, settings.data.adsr.release);
            break;

        case 73:  // adsr attack
            settings.data.adsr.attack = buf[1];
            settings.pending.adsr.attack = true;
            if (adsr_set_attack(&adsr, settings.data.adsr.attack))
                screen_set_adsr_attack(&screen, settings.data.adsr.attack);
            break;

        case 75:  // adsr decay
            settings.data.adsr.decay = buf[1];
            settings.pending.adsr.decay = true;
            if (adsr_set_decay(&adsr, settings.data.adsr.decay))
                screen_set_adsr_decay(&screen, settings.data.adsr.decay);
            break;

        case 79:  // adsr sustain
            settings.data.adsr.sustain = buf[1];
            settings.pending.adsr.sustain = true;
            if (adsr_set_sustain(&adsr, settings.data.adsr.sustain))
                screen_set_adsr_sustain(&screen, settings.data.adsr.sustain);
            break;

        case 102:  // midi channel
            if (buf[1] > 0x3f) {
                settings.data.midi_channel = ch;
                settings.pending.midi_channel = true;
                screen_set_midi_channel(&screen, settings.data.midi_channel);
            }
            break;

        case 119:  // write settings
            if (buf[1] > 0x3f)
                settings_start_write(&settings);
            break;

        case 120:  // all sound off
        case 123:  // all notes off
            adsr_unset_gate(&adsr, true);
            break;
        }
        break;

    default:
        break;
    }
}


static inline void
timer_init(void)
{
    // number of cycles between each audio sample
    TCB0.CCMP = timer_tcb_ccmp;

    // enable timer without prescaler division
    TCB0.CTRLA = TCB_RUNSTDBY_bm | timer_tcb_clksel | TCB_ENABLE_bm;
}


int
main(void)
{
    clock_init();
    opamp_init();
    dac_init();
    timer_init();

    adsr_init(&adsr);
    midi_init(&midi, midi_channel_cb, NULL);
    oscillator_init(&oscillator);
    screen_init(&screen);

    if (settings_init(&settings, &factory_settings)) {
        screen_set_midi_channel(&screen, settings.data.midi_channel);

        oscillator_set_waveform(&oscillator, settings.data.oscillator.waveform);
        screen_set_oscillator_waveform(&screen, settings.data.oscillator.waveform);

        adsr_set_type(&adsr, settings.data.adsr.type);
        screen_set_adsr_type(&screen, settings.data.adsr.type);

        adsr_set_attack(&adsr, settings.data.adsr.attack);
        screen_set_adsr_attack(&screen, settings.data.adsr.attack);

        adsr_set_decay(&adsr, settings.data.adsr.decay);
        screen_set_adsr_decay(&screen, settings.data.adsr.decay);

        adsr_set_sustain(&adsr, settings.data.adsr.sustain);
        screen_set_adsr_sustain(&screen, settings.data.adsr.sustain);

        adsr_set_release(&adsr, settings.data.adsr.release);
        screen_set_adsr_release(&screen, settings.data.adsr.release);
    }

    while (1) {
        if (TCB0.INTFLAGS & TCB_CAPT_bm) {
            TCB0.INTFLAGS = TCB_CAPT_bm;

            midi_task(&midi);
            screen_task(&screen);
            if (settings_task(&settings))
                screen_notification(&screen, SCREEN_NOTIFICATION_PRESET_UPDATED);

            DAC0.DATA = (amplifier_get_sample(oscillator_get_sample(&oscillator),
                adsr_get_sample_level(&adsr), velocity) + output_offset) << DAC_DATA_0_bp;
        }
    }

    return 0;
}
