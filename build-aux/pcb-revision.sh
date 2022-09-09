#!/bin/bash

ROOTDIR="$(realpath "$(dirname "${0}")/../")"

exec grep "(rev " "${ROOTDIR}/pcb/db-synth.kicad_pcb" | sed 's/.*rev "\([0-9.]*\)".*/\1/'
