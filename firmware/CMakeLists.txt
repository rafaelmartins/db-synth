# SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.12)

include(./cmake/avr.cmake)

project(db-synth C)

add_executable(db-synth
    main.c
    main-data.h

    adsr.c
    adsr.h
    adsr-data.h

    midi.c
    midi.h

    oled.c
    oled.h
    oled-font.c

    oscillator.c
    oscillator.h
    oscillator-data.h

    phase.h
)

target_compile_options(db-synth PRIVATE
    -Wall
    -Wextra
    -Werror
)

avr_target_set_device(db-synth avr128db28 24000000UL)
avr_target_show_size(db-synth avr128db28)
avr_target_gen_map(db-synth)
avr_target_generate_hex(db-synth)
avr_target_generate_fuse_hex(db-synth)
