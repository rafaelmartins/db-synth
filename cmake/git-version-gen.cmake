# SPDX-FileCopyrightText: 2024 Rafael G. Martins <rafael@rafaelmartins.eng.br>
# SPDX-License-Identifier: BSD-3-Clause

find_package(Git)

if (EXISTS "${CMAKE_SOURCE_DIR}/version.cmake")
    include("${CMAKE_SOURCE_DIR}/version.cmake")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/.git" AND Git_FOUND)
    execute_process(
        COMMAND
            ${GIT_EXECUTABLE}
                describe
                    --abbrev=4
                    --match "v[0-9]*"
                    HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE _gvg_version
        ERROR_VARIABLE _gvg_version_err
        RESULT_VARIABLE _gvg_version_result
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(_gvg_version_err)
        message(FATAL_ERROR "Failed to find version from Git\n${_gvg_version_err}")
    endif()

    if(NOT _gvg_version_result EQUAL 0)
        message(FATAL_ERROR "Failed to find version from Git: Git process returned ${_gvg_version_result}")
    endif()

    if(NOT _gvg_version)
        message(FATAL_ERROR "Failed to find version from Git: Empty")
    endif()

    string(REPLACE "-" ";" _gvg_version_split "${_gvg_version}")
    list(LENGTH _gvg_version_split _gvg_version_split_len)
    if(_gvg_version_split_len)
        list(GET _gvg_version_split 0 PACKAGE_VERSION_CANONICAL)
        string(REGEX REPLACE "^v" "" PACKAGE_VERSION_CANONICAL "${PACKAGE_VERSION_CANONICAL}")
        set(PACKAGE_VERSION_GIT "${PACKAGE_VERSION_CANONICAL}")
        if(_gvg_version_split_len GREATER 1)
            list(GET _gvg_version_split 1 _gvg_version_rev)
            set(PACKAGE_VERSION_GIT "${PACKAGE_VERSION_GIT}.${_gvg_version_rev}")
            if(_gvg_version_split_len GREATER 2)
                list(GET _gvg_version_split 2 _gvg_version_hash)
                string(REGEX REPLACE "^g" "" _gvg_version_hash "${_gvg_version_hash}")
                set(PACKAGE_VERSION_GIT "${PACKAGE_VERSION_GIT}-${_gvg_version_hash}")
            endif()
        endif()
    endif()

    if(NOT PACKAGE_VERSION_CANONICAL)
        message(FATAL_ERROR "Failed to find canonical version")
    endif()

    execute_process(
        COMMAND
            ${GIT_EXECUTABLE}
                update-index
                    -q
                    --refresh
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )

    execute_process(
        COMMAND
            ${GIT_EXECUTABLE}
                diff-index
                    --name-only
                    HEAD
                    --
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE _gvg_diff_index
        ERROR_VARIABLE _gvg_diff_index_err
        RESULT_VARIABLE _gvg_diff_index_result
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT _gvg_diff_index_err AND NOT _gvg_diff_index_result AND _gvg_diff_index)
        set(PACKAGE_VERSION_GIT "${PACKAGE_VERSION_GIT}-dirty")
    endif()
else()
    message(FATAL_ERROR "Can't find version information!")
endif()

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/cpack/install_version.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/install_version.cmake
    @ONLY
)

list(APPEND CPACK_INSTALL_SCRIPTS
    "${CMAKE_CURRENT_BINARY_DIR}/install_version.cmake"
)
