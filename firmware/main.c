/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <avr/io.h>
#include "adsr.h"
#include "filter.h"
#include "midi.h"
#include "oscillator.h"
#include "screen.h"
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
    if (ch != 0)
        return;

    switch (cmd) {
    case MIDI_NOTE_ON:
        if (len == 2 && buf[0] != 0) {
            oscillator_set_note(&oscillator, buf[0]);
            adsr_set_velocity(&adsr, buf[1]);
            adsr_set_gate(&adsr);
            break;
        }

    // fall through
    case MIDI_NOTE_OFF:
        adsr_unset_gate(&adsr);
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

    // FIXME: test settings, remove
    screen_set_midi_channel(&screen, 0);
    oscillator_set_waveform(&oscillator, OSCILLATOR_WAVEFORM_SINE);
    screen_set_oscillator_waveform(&screen, OSCILLATOR_WAVEFORM_SINE);
    adsr_set_attack(&adsr, 5);
    screen_set_adsr_attack(&screen, 5);
    adsr_set_decay(&adsr, 5);
    screen_set_adsr_decay(&screen, 5);
    adsr_set_sustain(&adsr, 100);
    screen_set_adsr_sustain(&screen, 100);
    adsr_set_release(&adsr, 5);
    screen_set_adsr_release(&screen, 5);
    filter_set_type(&filter, FILTER_TYPE_LOW_PASS);
    screen_set_filter_type(&screen, FILTER_TYPE_LOW_PASS);
    filter_set_cutoff(&filter, 64);
    screen_set_filter_cutoff(&screen, 64);

    while (1) {
        if (TCB0.INTFLAGS & TCB_CAPT_bm) {
            TCB0.INTFLAGS = TCB_CAPT_bm;

            midi_task(&midi);
            screen_task(&screen);

            DAC0.DATA = (filter_get_sample(&filter, adsr_get_sample(&adsr, oscillator_get_sample(&oscillator)))
                + oscillator_waveform_amplitude) << DAC_DATA_0_bp;
        }
    }

    return 0;
}
