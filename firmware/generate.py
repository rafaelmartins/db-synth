#!/usr/bin/env python3
'''
db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the
          AVR DB microcontroller series.

SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
SPDX-License-Identifier: BSD-3-Clause
'''

import itertools
import math
import os

'''
Global Settings
'''
cpu_frequency = 24000000
audio_sample_rate = 48000


if cpu_frequency != 24000000:
    raise RuntimeError('unsupported cpu frequency')

cpu_frqsel = 'CLKCTRL_FRQSEL_24M_gc'


'''
Bandlimited wavetables

References:
    - Tim Stilson and Julius Smith. 1996. Alias-Free Digital Synthesis of Classic
      Analog Waveforms (https://ccrma.stanford.edu/~stilti/papers/blit.pdf)
'''
a4_midi_number = 69
a4_frequency = 440.0

oscillator_waveform_amplitude = 0x1ff
oscillator_waveform_samples_per_cycle = 0x200
oscillator_note_frequencies = [a4_frequency * 2 ** ((i - a4_midi_number) / 12)
                               for i in range(128)]
oscillator_note_steps = [(oscillator_waveform_samples_per_cycle / (audio_sample_rate / i)) * (1 << 16)
                         for i in oscillator_note_frequencies]

# the octave closer to the nyquist frequency is usually a sine, then we reuse
# the existing sine wavetable.
oscillator_wavetable_octaves = math.ceil(len(oscillator_note_frequencies) / 12) - 1


def _fix_wavetable(array):
    mn = min(array)
    mx = max(array)

    for i in range(len(array)):
        array[i] -= mn
        array[i] *= (2 * oscillator_waveform_amplitude) / abs(mx - mn)
        array[i] = int(array[i]) - oscillator_waveform_amplitude

    return array[::-1]


oscillator_wavetables = {
    'sine': [oscillator_waveform_amplitude * math.sin(2 * math.pi * i / oscillator_waveform_samples_per_cycle)
             for i in range(oscillator_waveform_samples_per_cycle)],
    'sawtooth': [],
    'square': [],
    'triangle': [],
}

for i in range(oscillator_wavetable_octaves):
    idx = i * 12 + 11
    f = oscillator_note_frequencies[idx if idx < len(oscillator_note_frequencies)
                                    else len(oscillator_note_frequencies) - 1]

    P = audio_sample_rate / f
    M = 2 * math.floor(P / 2) + 1

    mid = 0
    blit = []
    for i in range(oscillator_waveform_samples_per_cycle):
        x = (i - oscillator_waveform_samples_per_cycle / 2) / oscillator_waveform_samples_per_cycle
        try:
            blit.append(math.sin(math.pi * x * M) / (M * math.sin(math.pi * x)))
        except ZeroDivisionError:
            mid = i
            blit.append(1.0)
    blit_avg = min(blit) + ((max(blit) - min(blit)) / 2)

    y = 0
    square = []
    for i in range(len(blit)):
        y += blit[i] - blit[i + mid if i < mid else i - mid]
        square.append(y)
    square_avg = min(square) + ((max(square) - min(square)) / 2)
    oscillator_wavetables['square'].append(_fix_wavetable(square))

    y = 0
    triangle = []
    for v in square:
        y += v - square_avg
        triangle.append(y)
    triangle_start = oscillator_waveform_samples_per_cycle // 4
    triangle = triangle[triangle_start:] + triangle[:triangle_start]
    oscillator_wavetables['triangle'].append(_fix_wavetable(triangle))

    y = 0
    sawtooth = []
    for i in range(len(blit)):
        y += blit[i + mid if i < mid else i - mid] - 1. / P
        sawtooth.append(-y)
    oscillator_wavetables['sawtooth'].append(_fix_wavetable(sawtooth))


'''
ADSR envelope
'''
adsr_amplitude = 0xff
adsr_samples_per_cycle = oscillator_waveform_samples_per_cycle

# we try to emulate the curves from AS3310.
# this is from page 2 of datasheet.
adsr_attack_asymptote_voltage = 7.0
adsr_attack_peak_voltage = 5.0

# time input data also based on AS3310 as much as possible.
adsr_times_len = 0x100
adsr_times_min_ms = 2
adsr_times_max_ms = 20000

adsr_levels_len = 0x100

adsr_time_description_strlen = 6
adsr_level_description_strlen = 6

adsr_t = [i / (adsr_samples_per_cycle - 1) for i in range(adsr_samples_per_cycle)]
adsr_full_curve = [1 - math.exp(-3 * i) for i in adsr_t]

adsr_attack_max = 0
for i, v in enumerate(adsr_full_curve):
    if v / adsr_full_curve[-1] >= adsr_attack_peak_voltage / adsr_attack_asymptote_voltage:
        adsr_attack_max = adsr_t[i]
        break

adsr_attack_curve = [1 - math.exp(-3 * i * adsr_attack_max) for i in adsr_t]
adsr_attack_curve = [int(adsr_amplitude * i / adsr_attack_curve[-1]) for i in adsr_attack_curve]

adsr_decay_release_curve = [int(adsr_amplitude * i / adsr_full_curve[-1]) for i in adsr_full_curve]

adsr_curves = {
    'attack': adsr_attack_curve,
    'decay_release': adsr_decay_release_curve,
}

adsr_times_end = adsr_times_max_ms - adsr_times_min_ms

adsr_times = [-1 + math.exp(6 * i / (adsr_times_len - 1)) for i in range(adsr_times_len)]
adsr_times = [adsr_times_min_ms + int(adsr_times_end * i / adsr_times[-1]) for i in adsr_times]

adsr_time_steps = [((adsr_samples_per_cycle * 1000) / (i * audio_sample_rate)) * (1 << 16) for i in adsr_times]

adsr_time_descriptions = [('%.2fs' % (i / 1000)) if i > 1000 else ('%dms' % i) for i in adsr_times]
adsr_level_descriptions = ['%.1f%%' % (100. * i / (adsr_levels_len - 1)) for i in range(adsr_levels_len)]


'''
1-Pole filters
'''

filter_frequencies_len = 0x100
filter_frequencies_min = 20
filter_frequencies_max = 20000

filter_cutoff_description_strlen = 8

filter_frequencies_end = filter_frequencies_max - filter_frequencies_min

filter_frequencies = [-1 + math.exp(3 * i / (filter_frequencies_len - 1)) for i in range(filter_frequencies_len)]
filter_frequencies = [filter_frequencies_min + int(filter_frequencies_end * i / filter_frequencies[-1]) for i in filter_frequencies]

filter_lp_a1 = []
filter_lp_b0 = []
filter_lp_b1 = []
filter_hp_a1 = []
filter_hp_b0 = []
filter_hp_b1 = []

for f in filter_frequencies:
    alpha = (2 * math.pi * f) / audio_sample_rate

    filter_lp_a1.append(-(alpha - 2) / (alpha + 2))
    filter_lp_b0.append(alpha / (alpha + 2))
    filter_lp_b1.append(alpha / (alpha + 2))

    filter_hp_a1.append((1 - (alpha / 2)) / (1 + (alpha / 2)))
    filter_hp_b0.append(1 / (1 + (alpha / 2)))
    filter_hp_b1.append(-1 / (1 + (alpha / 2)))

filter_cutoff_descriptions = [('%.2fkHz' % (i / 1000)) if i > 1000 else ('%dHz' % i) for i in filter_frequencies]


'''
MIDI (AVR USART)
'''
midi_usart_baudrate = 31250

midi_usart_baud = (64 * cpu_frequency) // (16 * midi_usart_baudrate)

# control flags that affect the baud formula
midi_usart_rxmode = 'USART_RXMODE_NORMAL_gc'
midi_usart_cmode = 'USART_CMODE_ASYNCHRONOUS_gc'


'''
OLED (AVR TWI)
'''
oled_twi_baudrate = 400000
oled_twi_rise_time = 400e-9

oled_twi_mbaud = int((cpu_frequency / (2 * oled_twi_baudrate)) - (5 + ((cpu_frequency * oled_twi_rise_time) / 2)))


'''
OPAMPs
'''
opamp_timebase = int((cpu_frequency * 1e-6) - 1)


'''
TIMER (AVR TCB)
'''
timer_tcb_ccmp = cpu_frequency // audio_sample_rate

# control flags that affect ccmp
timer_tcb_clksel = 'TCB_CLKSEL_DIV1_gc'


'''
Helpers and renderers
'''

def format_hex(v, zero_padding=4):
    return '%s0x%0*x' % (int(v) < 0 and '-' or '', zero_padding, abs(int(v)))


def header():
    yield '/*'
    yield ' * db-synth: A MIDI-controlled mono-voice digital synthesizer built on top of the'
    yield ' *           AVR DB microcontroller series.'
    yield ' *'
    yield ' * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>'
    yield ' * SPDX-License-Identifier: BSD-3-Clause'
    yield ' */'
    yield ''
    yield '// this file was generated by generate.py. do not edit!'
    yield ''
    yield '#pragma once'


def dump_headers(headers, system=True):
    if headers:
        yield ''

    for header in headers:
        yield '#include %s%s%s' % (system and '<' or '"', header, system and '>' or '"')


def dump_macros(items):
    if items:
        yield ''

    for item in items:
        yield '#define %s %s' % (item, items[item])


def dump_oscillator_notes():
    yield ''
    yield 'static const uint32_t oscillator_notes[] PROGMEM = {'

    for i in range(len(oscillator_note_steps) // 4):
        yield '    %s,' % ', '.join([format_hex(j, 8)
                                     for j in oscillator_note_steps[i * 4: (i + 1) * 4]])

    yield '};'


def dump_oscillator_wavetables():
    for var, array in oscillator_wavetables.items():
        if len(array) == 0:
            continue

        yield ''

        if not isinstance(array[0], list):
            yield 'static const int16_t oscillator_%s_wavetable[%s] PROGMEM = {' % \
                (var, format_hex(len(array)))
            for i in range(len(array) // 8):
                yield '    %s,' % ', '.join([format_hex(j)
                                             for j in array[i * 8: (i + 1) * 8]])
            yield '};'
            continue

        yield 'static const int16_t oscillator_%s_wavetables[%d][%s] PROGMEM = {' % \
            (var, len(array), format_hex(len(array[0])))
        for value in array:
            yield '    {'
            for i in range(len(value) // 8):
                yield '        %s,' % ', '.join([format_hex(j)
                                                 for j in value[i * 8: (i + 1) * 8]])
            yield '    },'
        yield '};'


def dump_adsr_curves():
    for var, value in adsr_curves.items():
        yield ''
        yield 'static const uint8_t adsr_%s_curve[] PROGMEM = {' % var
        for i in range(len(value) // 8):
            yield '    %s,' % ', '.join([format_hex(j, 2)
                                         for j in value[i * 8: (i + 1) * 8]])
        yield '};'


def dump_adsr_time_steps():
    yield ''
    yield 'static const uint32_t adsr_time_steps[] PROGMEM = {'

    for i in range(len(adsr_time_steps) // 4):
        yield '    %s,' % ', '.join([format_hex(j, 8)
                                     for j in adsr_time_steps[i * 4: (i + 1) * 4]])

    yield '};'


def dump_adsr_time_descriptions():
    yield ''
    yield 'static const char adsr_time_descriptions[%d][%d] PROGMEM = {' % (len(adsr_times), adsr_time_description_strlen)

    for i in range(len(adsr_time_descriptions) // 8):
        yield '    %s,' % ', '.join(['"%*s"' % (-adsr_time_description_strlen, j)
                                     for j in adsr_time_descriptions[i * 8: (i + 1) * 8]])

    yield '};'


def dump_adsr_level_descriptions():
    yield ''
    yield 'static const char adsr_level_descriptions[%d][%d] PROGMEM = {' % (adsr_levels_len, adsr_level_description_strlen)

    for i in range(len(adsr_level_descriptions) // 8):
        yield '    %s,' % ', '.join(['"%*s"' % (-adsr_level_description_strlen, j)
                                     for j in adsr_level_descriptions[i * 8: (i + 1) * 8]])

    yield '};'


def dump_filter_coefficients(var, name):
    yield ''
    yield 'static const uint8_t %s[] PROGMEM = {' % name

    for i in range(len(var) // 8):
        yield '    %s,' % ', '.join([format_hex(j * (1 << 7), 2)
                                     for j in var[i * 8: (i + 1) * 8]])

    yield '};'


def dump_filter_cuttoff_descriptions():
    yield ''
    yield 'static const char filter_cutoff_descriptions[%d][%d] PROGMEM = {' % (len(filter_frequencies), filter_cutoff_description_strlen)

    for i in range(len(filter_cutoff_descriptions) // 8):
        yield '    %s,' % ', '.join(['"%*s"' % (-filter_cutoff_description_strlen, j)
                                     for j in filter_cutoff_descriptions[i * 8: (i + 1) * 8]])

    yield '};'


generators = {
    'main-data.h': itertools.chain(
        header(),
        dump_headers(['avr/io.h']),
        dump_macros({
            'cpu_frqsel': cpu_frqsel,
            'opamp_timebase': opamp_timebase,
            'timer_tcb_ccmp': timer_tcb_ccmp,
            'timer_tcb_clksel': timer_tcb_clksel,
            'oscillator_waveform_amplitude': format_hex(oscillator_waveform_amplitude),
        }),
    ),
    'adsr-data.h': itertools.chain(
        header(),
        dump_headers(['avr/pgmspace.h', 'stdint.h']),
        dump_macros({
            'adsr_amplitude': format_hex(adsr_amplitude, 2),
            'adsr_samples_per_cycle': format_hex(adsr_samples_per_cycle),
        }),
        dump_adsr_curves(),
        dump_adsr_time_steps(),
    ),
    'filter-data.h': itertools.chain(
        header(),
        dump_headers(['avr/pgmspace.h', 'stdint.h']),
        dump_filter_coefficients(filter_lp_a1, 'filter_lp_a1'),
        dump_filter_coefficients(filter_lp_b0, 'filter_lp_b0'),
        dump_filter_coefficients(filter_lp_b1, 'filter_lp_b1'),
        dump_filter_coefficients(filter_hp_a1, 'filter_hp_a1'),
        dump_filter_coefficients(filter_hp_b0, 'filter_hp_b0'),
        dump_filter_coefficients(filter_hp_b1, 'filter_hp_b1'),
    ),
    'midi-data.h': itertools.chain(
        header(),
        dump_headers(['avr/io.h']),
        dump_macros({
            'midi_usart_baud': midi_usart_baud,
            'midi_usart_rxmode': midi_usart_rxmode,
            'midi_usart_cmode': midi_usart_cmode,
        }),
    ),
    'oled-data.h': itertools.chain(
        header(),
        dump_macros({
            'oled_twi_mbaud': oled_twi_mbaud,
        }),
    ),
    'oscillator-data.h': itertools.chain(
        header(),
        dump_headers(['avr/pgmspace.h', 'stdint.h']),
        dump_macros({
            'oscillator_waveform_samples_per_cycle': format_hex(oscillator_waveform_samples_per_cycle),
            'oscillator_wavetable_octaves': oscillator_wavetable_octaves,
            'oscillator_notes_last': len(oscillator_note_frequencies) - 1,
        }),
        dump_oscillator_wavetables(),
        dump_oscillator_notes(),
    ),
    'screen-data.h': itertools.chain(
        header(),
        dump_macros({
            'adsr_time_description_strlen': adsr_time_description_strlen,
            'adsr_level_description_strlen': adsr_level_description_strlen,
            'filter_cutoff_description_strlen': filter_cutoff_description_strlen,
        }),
        dump_adsr_time_descriptions(),
        dump_adsr_level_descriptions(),
        dump_filter_cuttoff_descriptions(),
    ),
}


if __name__ == '__main__':
    rootdir = os.path.dirname(os.path.abspath(__file__))
    for key in generators:
        print('generating %s ...' % key)

        with open(os.path.join(rootdir, key), 'w') as fp:
            for l in generators[key]:
                print(l, file=fp)
