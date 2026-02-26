# SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
# SPDX-License-Identifier: BSD-3-Clause

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/avr-toolchain.cmake")

function(avr_target_set_device target mcu f_cpu)
    target_compile_options(${target}
        PRIVATE "-mmcu=${mcu}"
    )
    target_compile_definitions(${target}
        PRIVATE "-DF_CPU=${f_cpu}"
    )
    target_link_options(${target}
        PRIVATE "-mmcu=${mcu}"
    )
endfunction()

function(avr_target_gen_map target)
    target_link_options(${target}
        PRIVATE "-Wl,-Map,$<TARGET_FILE:${target}>.map"
    )
    set_property(TARGET ${target}
        APPEND
        PROPERTY ADDITIONAL_CLEAN_FILES "$<TARGET_FILE:${target}>.map"
    )
endfunction()

function(avr_target_generate_hex target)
    add_custom_command(
        OUTPUT ${target}.hex
        COMMAND ${AVR_OBJCOPY} -j .text -j .data -O ihex $<TARGET_FILE:${target}> ${target}.hex
        DEPENDS $<TARGET_FILE:${target}>
    )
    add_custom_target(${target}-hex
        ALL
        DEPENDS ${target}.hex
    )
endfunction()

function(avr_target_generate_eeprom_hex target)
    add_custom_command(
        OUTPUT ${target}-eeprom.hex
        COMMAND ${AVR_OBJCOPY} -j .eeprom -O ihex $<TARGET_FILE:${target}> ${target}-eeprom.hex
        DEPENDS $<TARGET_FILE:${target}>
    )
    add_custom_target(${target}-eeprom-hex
        ALL
        DEPENDS ${target}-eeprom.hex
    )
endfunction()

function(avr_target_generate_fuse_hex target)
    add_custom_command(
        OUTPUT ${target}-fuse.hex
        COMMAND ${AVR_OBJCOPY} -j .fuse -O ihex $<TARGET_FILE:${target}> ${target}-fuse.hex
        DEPENDS $<TARGET_FILE:${target}>
    )
    add_custom_target(${target}-fuse-hex
        ALL
        DEPENDS ${target}-fuse.hex
    )
endfunction()

function(avr_target_show_size target mcu)
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${AVR_SIZE} -C --mcu=${mcu} "$<TARGET_FILE:${target}>"
    )
endfunction()
