#!/bin/bash

set -e

if [[ -z "${1}" ]]; then
    echo "error: missing output file"
    exit 1
fi

MYDIR="$(realpath "$(dirname "${0}")")"

mkdir -p "$(dirname "${1}")"

cat <<EOF > "${1}"
db-synth
========

Version:      $("${MYDIR}/version.sh")
PCB Revision: $("${MYDIR}/pcb-revision.sh")

Files:
    avr64db28/: Firmware files for AVR64DB28 microcontroller
        db-synth.elf: ELF binary
        db-synth.elf.map: ELF binary map file
        db-synth.hex: Flash data HEX file
        db-synth-fuse.hex: Fuse data HEX file
        size.txt: Memory usage report
    avr128db28/: Firmware files for AVR128DB28 microcontroller
        db-synth.elf: ELF binary
        db-synth.elf.map: ELF binary map file
        db-synth.hex: Flash data HEX file
        db-synth-fuse.hex: Fuse data HEX file
        size.txt: Memory usage report
    3d-models/: 3D models
        enclosure-back.stl: Enclosure (back)
        enclosure-front.stl: Enclosure (front)
    pcb/: Printed circuit board resources
        ibom.html: Kicad PCB interactive bill of materials
        schematics.pdf: Kicad schematics
    midi.pdf: Midi Implementation Chart

TODO
EOF
