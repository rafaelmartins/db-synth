# SPDX-FileCopyrightText: 2022-2026 Rafael G. Martins <rafael@rafaelmartins.eng.br>
# SPDX-License-Identifier: BSD-3-Clause

include(CheckIPOSupported)

add_executable(db-synth
    main.c
    adsr.c
    amplifier.c
    filter.c
    midi.c
    oled.c
    oscillator.c
    screen.c
    settings.c
)

target_compile_definitions(db-synth PRIVATE
    DB_SYNTH_VERSION=\"${PACKAGE_VERSION_GIT}\"
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

install(TARGETS db-synth)
install(FILES
    "$<TARGET_FILE:db-synth>.map"
    "${CMAKE_CURRENT_BINARY_DIR}/db-synth.hex"
    "${CMAKE_CURRENT_BINARY_DIR}/db-synth-fuse.hex"
    TYPE BIN
)
