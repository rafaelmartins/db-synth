# SPDX-FileCopyrightText: 2024 Rafael G. Martins <rafael@rafaelmartins.eng.br>
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.25)

include(cmake/avr.cmake)
include(cmake/git-version-gen.cmake)

project(db-synth
    VERSION "${PACKAGE_VERSION_CANONICAL}"
    HOMEPAGE_URL "https://rafaelmartins.com/p/db-synth/"
    LANGUAGES C ASM
)

option(WITH_MCU "AVR Microcontroller.")
if(NOT WITH_MCU)
    set(WITH_MCU "avr128db28" CACHE STRING "AVR Microcontroller." FORCE)
endif()

add_subdirectory(firmware)

configure_file(
    cmake/cpack/files/README.txt.in
    cmake/cpack/files/README.txt
    @ONLY
)

install(FILES
    cmake/cpack/files/LICENSE.txt
    "${CMAKE_BINARY_DIR}/cmake/cpack/files/README.txt"
    DESTINATION .
)

set(CPACK_VERBATIM_VARIABLES YES)

set(CPACK_GENERATOR TXZ ZIP)
set(CPACK_PACKAGE_VENDOR "Rafael G. Martins")
set(CPACK_PACKAGE_VERSION "${PACKAGE_VERSION_GIT}")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${WITH_MCU}-${PACKAGE_VERSION_GIT}")
set(CPACK_PACKAGE_CHECKSUM SHA512)

set(CPACK_SOURCE_GENERATOR TXZ ZIP)
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-source-${PACKAGE_VERSION_GIT}")
set(CPACK_SOURCE_IGNORE_FILES
    \\.git
    \\.gitignore
    \\.github/
    "build.*/"
    "/.*-backups/"
    "/gerber/"
    "-cache$"
    "-cache.lib$"
    "\\.(000|bak|bck|kicad_pcb-bak|lck|tmp|dsn|ses|kicad_sch-bak|kicad_prl)$"
    ".*~$"
)

include(CPack)
