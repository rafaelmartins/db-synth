# SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.12)

include(CheckIPOSupported)
include(./cmake/avr.cmake)

project(db-synth C)

option(WITH_MCU "AVR Microcontroller.")
if(NOT WITH_MCU)
    set(WITH_MCU "avr128db28" CACHE STRING "AVR Microcontroller" FORCE)
endif()

execute_process(
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../build-aux/version.sh"
    OUTPUT_VARIABLE DB_SYNTH_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_executable(db-synth
    main.c
    main-data.h

    adsr.c
    adsr.h
    adsr-data.h

    amplifier.c
    amplifier.h

    midi.c
    midi.h
    midi-data.h

    oled.c
    oled.h
    oled-data.h
    oled-font.h

    oscillator.c
    oscillator.h
    oscillator-data.h

    screen.c
    screen.h
    screen-data.h

    settings.c
    settings.h
)

target_compile_definitions(db-synth PRIVATE
    DB_SYNTH_VERSION=\"${DB_SYNTH_VERSION}\"
)

target_compile_options(db-synth PRIVATE
    -Wall
    -Wextra
    -Werror
)

check_ipo_supported()
set_property(TARGET db-synth PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

avr_target_set_device(db-synth "${WITH_MCU}" 24000000UL)
avr_target_show_size(db-synth "${WITH_MCU}")
avr_target_gen_map(db-synth)
avr_target_generate_hex(db-synth)
avr_target_generate_fuse_hex(db-synth)
