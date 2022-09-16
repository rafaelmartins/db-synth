#!/bin/bash

set -e

if [[ -z "${1}" ]]; then
    echo "error: missing output dir"
    exit 1
fi

if [[ x$CI = xtrue ]]; then
    sudo apt install -y ninja-build

    wget -q "https://github.com/rafaelmartins/avr-toolchain/releases/download/avr-toolchain-linux-amd64-${AVR_TOOLCHAIN_TIMESTAMP}/avr-toolchain-linux-amd64-${AVR_TOOLCHAIN_TIMESTAMP}.tar.xz"{,.sha512}
    sha512sum -c "avr-toolchain-linux-amd64-${AVR_TOOLCHAIN_TIMESTAMP}.tar.xz.sha512"
    tar -xf "avr-toolchain-linux-amd64-${AVR_TOOLCHAIN_TIMESTAMP}.tar.xz"

    export PATH="$(pwd)/avr/bin:${PATH}"
fi

MYDIR="$(realpath "$(dirname "${0}")")"
ROOTDIR="$(realpath "${MYDIR}/../")"
SRCDIR="${ROOTDIR}/firmware"
BUILDDIR="${ROOTDIR}/build"
PREFIXDIR="${1}"

build_for() {
    local builddir="${BUILDDIR}/${1}"
    local prefixdir="${PREFIXDIR}/${1}"

    mkdir -p "${builddir}" "${prefixdir}"

    cmake \
        -D WITH_MCU="${1}" \
        -D CMAKE_BUILD_TYPE=Release \
        -S "${SRCDIR}" \
        -B "${builddir}" \
        -G Ninja

    cmake \
        --build "${builddir}" \
        --target all

    avr-size \
        -C \
        --mcu="${1}" \
        "${builddir}/db-synth.elf" > "${prefixdir}/size.txt"

    cp "${builddir}/db-synth"* "${prefixdir}/"
}

rm -rf "${BUILDDIR}"

build_for avr64db28
build_for avr128db28
