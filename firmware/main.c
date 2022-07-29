/*
 * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
 *           AVR DB microcontroller series.
 *
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <avr/interrupt.h>
#include <avr/io.h>
#include "adsr.h"
#include "midi.h"
#include "oled.h"
#include "oscillator.h"
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

static volatile adsr_t adsr;
static volatile midi_t midi;
static volatile oled_t oled;
static volatile oscillator_t oscillator;


ISR(TCB0_INT_vect)
{
    TCB0.INTFLAGS = TCB_CAPT_bm;

    midi_task(&midi);
    oled_task(&oled);

    DAC0.DATA = (adsr_sample(&adsr, oscillator_get_sample(&oscillator)) + waveform_amplitude) << DAC_DATA_0_bp;
}


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
midi_channel_cb(midi_command_t cmd, uint8_t ch, volatile uint8_t *buf, uint8_t len)
{
    if (ch != 0)
        return;

    switch (cmd) {
    case MIDI_NOTE_ON:
        if (len == 2 && buf[0] != 0) {
            oscillator_set_note(&oscillator, buf[0]);
            adsr_set_velocity(&adsr, buf[1]);
            adsr_set_gate(&adsr);

            // FIXME: test code, results in some audio clicks
            // char b[5];
            // b[0] = '0';
            // b[1] = 'x';
            // b[2] = (buf[0] >> 4) > 10 ? (buf[0] >> 4) - 10 + 'a' : (buf[0] >> 4) + '0';
            // b[3] = (buf[0] & 0xf) > 10 ? (buf[0] & 0xf) - 10 + 'a' : (buf[0] & 0xf) + '0';
            // b[4] = 0;
            // oled_line(3, b, OLED_HALIGN_CENTER);

            return;
        }

    // fall through
    case MIDI_NOTE_OFF:
        adsr_unset_gate(&adsr);
        return;

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

    // enable capture interrupt
    TCB0.INTCTRL = TCB_CAPT_bm;

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
    oscillator_init(&oscillator);

    oled_init(&oled);
    oled_line(&oled, 0, "db-synth", OLED_HALIGN_CENTER);

    // FIXME: test settings, remove
    oscillator_set_waveform(&oscillator, OSCILLATOR_WAVEFORM_SINE);
    adsr_set_attack(&adsr, 10);
    adsr_set_decay(&adsr, 20);
    adsr_set_sustain(&adsr, 200);
    adsr_set_release(&adsr, 10);

    sei();

    while(1);

    return 0;
}
