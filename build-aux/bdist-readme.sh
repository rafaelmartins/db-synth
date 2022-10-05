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


How to program the microcontroller
----------------------------------

We can write to the microcontroller memory spaces using the UPDI interface.
It is recommended to use the pymcuprog tool [1], from Microchip itself, with a
Serial port UPDI adapter, that can be easily built using a TTL serial adapter
and a resistor, as described in the pymcuprog README.

With the adapter connected to the UPDI port in the db-synth PCB, and
configured as described in the pymcuprog documentation, run (assuming that the
serial adapter is available at /dev/ttyUSB0 and that it supports a baudrate of
230400):

For AVR64DB28:

    $ pymcuprog --tool uart --device avr64db28 --uart /dev/ttyUSB0 \\
        --clk 230400 write --filename avr64db28/db-synth-fuse.hex
    $ pymcuprog --tool uart --device avr64db28 --uart /dev/ttyUSB0 \\
        --clk 230400 write --filename avr64db28/db-synth.hex --erase

For AVR128DB28:

    $ pymcuprog --tool uart --device avr128db28 --uart /dev/ttyUSB0 \\
        --clk 230400 write --filename avr128db28/db-synth-fuse.hex
    $ pymcuprog --tool uart --device avr128db28 --uart /dev/ttyUSB0 \\
        --clk 230400 write --filename avr128db28/db-synth.hex --erase

The first command will write fuses, and the second command will write the
firmware to flash.

[1] https://github.com/microchip-pic-avr-tools/pymcuprog


Troubleshooting
---------------

If the pymcuprog command fails on Linux with some permission problem, make sure
to add the user to the group that owns the device /dev/ttyUSB0 (from the
example commands). It is possible to check which group is it, by running:

    $ ls -l /dev/ttyUSB0
    crw-rw---- 1 root dialout 188, 3 Aug 29 00:06 /dev/ttyUSB0

In this example, the group is dialout. To add current user to the group:

    $ sudo gpasswd -a \$USER dialout

EOF
