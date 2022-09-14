# SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.12)

include(./cmake/avr.cmake)

project(db-synth C)

option(WITH_MCU "AVR Microcontroller.")
if(NOT WITH_MCU)
    set(WITH_MCU "avr128db28" CACHE STRING "AVR Microcontroller" FORCE)
endif()

add_executable(db-synth
    main.c
    main-data.h

    adsr.h
    adsr-data.h

    amplifier.h

    filter.h
    filter-data.h

    midi.h
    midi-data.h

    oled.h
    oled-data.h
    oled-font.h

    oscillator.h
    oscillator-data.h

    phase.h

    screen.h
    screen-data.h

    settings.h
)

target_compile_options(db-synth PRIVATE
    -Wall
    -Wextra
    -Werror
)

avr_target_set_device(db-synth "${WITH_MCU}" 24000000UL)
avr_target_show_size(db-synth "${WITH_MCU}")
avr_target_gen_map(db-synth)
avr_target_generate_hex(db-synth)
avr_target_generate_fuse_hex(db-synth)
