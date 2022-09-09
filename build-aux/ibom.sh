#!/bin/bash

set -e

MYDIR="$(realpath "$(dirname "${0}")")"
ROOTDIR="$(realpath "${MYDIR}/../")"
MYTMPDIR="$(mktemp -d)"

trap 'rm -rf -- "${MYTMPDIR}"' EXIT

if [[ x$CI = xtrue ]]; then
    sudo add-apt-repository -y ppa:kicad/kicad-6.0-releases
    sudo apt update
    sudo apt install -y --no-install-recommends kicad

    export PATH="${ROOTDIR}/InteractiveHtmlBom/InteractiveHtmlBom:${PATH}"
fi

export INTERACTIVE_HTML_BOM_NO_DISPLAY=1

generate_interactive_bom.py \
    --dark-mode \
    --no-browser \
    --dest-dir "${MYTMPDIR}" \
    --name-format "%f" \
    --include-tracks \
    --include-nets \
    "${ROOTDIR}/pcb/db-synth.kicad_pcb"

cat "${MYTMPDIR}/db-synth.html"
