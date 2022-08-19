/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <avr/io.h>
#include "adsr.h"
#include "amplifier.h"
#include "filter.h"
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
static filter_t filter;
static midi_t midi;
static oscillator_t oscillator;
static screen_t screen;
static settings_t settings;
static uint8_t velocity;

static const settings_data_t factory_settings PROGMEM = {
    .version = SETTINGS_VERSION,
    .midi_channel = 0,
    .oscillator = {
        .waveform = OSCILLATOR_WAVEFORM_SQUARE,
    },
    .adsr = {
        .attack = 0x08,
        .decay = 0x08,
        .sustain = 0x60,
        .release = 0x08,
    },
    .filter = {
        .type = FILTER_TYPE_LOW_PASS,
        .cutoff = 0x3f,
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
            velocity = buf[1] * 2;
            adsr_set_gate(&adsr);
            break;
        }

    // fall through
    case MIDI_NOTE_OFF:
        // FIXME: validate current note
        adsr_unset_gate(&adsr);
        break;

    case MIDI_CONTROL_CHANGE:
        if (len != 2)
            break;

        switch (buf[0]) {
        case 0x03:  // waveform
            settings.data.oscillator.waveform = buf[1] / (0x80 / OSCILLATOR_WAVEFORM__LAST);
            if (settings.data.oscillator.waveform >= OSCILLATOR_WAVEFORM__LAST)
                settings.data.oscillator.waveform--;
            settings.pending.oscillator.waveform = true;
            if (oscillator_set_waveform(&oscillator, settings.data.oscillator.waveform))
                screen_set_oscillator_waveform(&screen, settings.data.oscillator.waveform);
            break;

        case 0x46:  // adsr type
            settings.data.adsr.type = buf[1] / (0x80 / ADSR_TYPE__LAST);
            if (settings.data.adsr.type >= ADSR_TYPE__LAST)
                settings.data.adsr.type--;
            settings.pending.adsr.type = true;
            if (adsr_set_type(&adsr, settings.data.adsr.type))
                screen_set_adsr_type(&screen, settings.data.adsr.type);
            break;

        case 0x47:  // filter type
            settings.data.filter.type = buf[1] / (0x80 / FILTER_TYPE__LAST);
            if (settings.data.filter.type >= FILTER_TYPE__LAST)
                settings.data.filter.type--;
            settings.pending.filter.type = true;
            if (filter_set_type(&filter, settings.data.filter.type))
                screen_set_filter_type(&screen, settings.data.filter.type);
            break;

        case 0x48:  // adsr release
            settings.data.adsr.release = buf[1];
            settings.pending.adsr.release = true;
            if (adsr_set_release(&adsr, settings.data.adsr.release))
                screen_set_adsr_release(&screen, settings.data.adsr.release);
            break;

        case 0x49:  // adsr attack
            settings.data.adsr.attack = buf[1];
            settings.pending.adsr.attack = true;
            if (adsr_set_attack(&adsr, settings.data.adsr.attack))
                screen_set_adsr_attack(&screen, settings.data.adsr.attack);
            break;

        case 0x4a:  // filter cutoff
            settings.data.filter.cutoff = buf[1];
            settings.pending.filter.cutoff = true;
            if (filter_set_cutoff(&filter, settings.data.filter.cutoff))
                screen_set_filter_cutoff(&screen, settings.data.filter.cutoff);
            break;

        case 0x4b:  // adsr decay
            settings.data.adsr.decay = buf[1];
            settings.pending.adsr.decay = true;
            if (adsr_set_decay(&adsr, settings.data.adsr.decay))
                screen_set_adsr_decay(&screen, settings.data.adsr.decay);
            break;

        case 0x4f:  // adsr sustain
            settings.data.adsr.sustain = buf[1];
            settings.pending.adsr.sustain = true;
            if (adsr_set_sustain(&adsr, settings.data.adsr.sustain))
                screen_set_adsr_sustain(&screen, settings.data.adsr.sustain);
            break;

        case 0x66:  // midi channel
            if (buf[1] > 0x3f) {
                settings.data.midi_channel = ch;
                settings.pending.midi_channel = true;
                screen_set_midi_channel(&screen, settings.data.midi_channel);
            }
            break;

        case 0x77:  // write settings
            if (buf[1] > 0x3f)
                settings_start_write(&settings);
            break;

        case 0x7b:  // all notes off
            adsr_unset_gate(&adsr);
            break;
        }
        break;

    default:
        break;
    }
}


static inline void
midi_init(void)
{
    midi_hw_init();
    midi.channel_cb = midi_channel_cb;
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
    midi_init();
    timer_init();

    adsr_init(&adsr);
    filter_init(&filter);
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

        filter_set_type(&filter, settings.data.filter.type);
        screen_set_filter_type(&screen, settings.data.filter.type);

        filter_set_cutoff(&filter, settings.data.filter.cutoff);
        screen_set_filter_cutoff(&screen, settings.data.filter.cutoff);
    }

    while (1) {
        if (TCB0.INTFLAGS & TCB_CAPT_bm) {
            TCB0.INTFLAGS = TCB_CAPT_bm;

            midi_task(&midi);
            screen_task(&screen);
            settings_task(&settings);

            DAC0.DATA = (filter_get_sample(&filter, amplifier_get_sample(oscillator_get_sample(&oscillator),
                adsr_get_sample_level(&adsr), velocity)) + oscillator_waveform_amplitude) << DAC_DATA_0_bp;
        }
    }

    return 0;
}
