# SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
# SPDX-License-Identifier: BSD-3-Clause

set(CMAKE_SYSTEM_NAME      Generic-ELF)
set(CMAKE_SYSTEM_PROCESSOR avr)
set(CMAKE_ASM_COMPILER     avr-gcc)
set(CMAKE_C_COMPILER       avr-gcc)
set(CMAKE_CXX_COMPILER     avr-g++)

find_program(AVR_SIZE    avr-size    REQUIRED)
find_program(AVR_OBJCOPY avr-objcopy REQUIRED)
find_program(AVR_OBJDUMP avr-objdump REQUIRED)

# as the elf is not transferred directly to the microncontroller, we can always have debug symbols included
set(CMAKE_ASM_FLAGS_INIT "-ggdb3 -funsigned-char -funsigned-bitfields -fshort-enums")
set(CMAKE_C_FLAGS_INIT   "-ggdb3 -funsigned-char -funsigned-bitfields -fshort-enums")
set(CMAKE_CXX_FLAGS_INIT "-ggdb3 -funsigned-char -funsigned-bitfields -fshort-enums")

# avr delay functions won't work without any optimization level enabled
string(APPEND CMAKE_ASM_FLAGS_DEBUG " -O1")
string(APPEND CMAKE_C_FLAGS_DEBUG   " -O1")
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O1")

set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
