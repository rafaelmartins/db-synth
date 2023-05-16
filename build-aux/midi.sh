#!/bin/bash

set -e

if [[ -z "${1}" ]]; then
    echo "error: missing output file"
    exit 1
fi

MYDIR="$(realpath "$(dirname "${0}")")"
ROOTDIR="$(realpath "${MYDIR}/../")"

if [[ x$CI = xtrue ]]; then
    sudo apt install -y wkhtmltopdf
fi

mkdir -p "$(dirname "${1}")"

cat <<EOF | wkhtmltopdf - "${1}"
<html>
<head>
<meta charset="UTF-8">
<title>MIDI Implementation Chart</title>
<style type="text/css">
h1 {
    text-align: center;
}
table {
    width: 100%;
}
table, th, td {
    border: 1px solid black;
}
th, td {
    padding: 5px;
}
</style>
</head>
<body>
<h1>MIDI Implementation Chart</h1>
<ul>
<li><strong>Product Name:</strong> db-synth</li>
<li><strong>Product Version:</strong> $("${MYDIR}/version.sh")</li>
<li><strong>PCB Revision:</strong> $("${MYDIR}/pcb-revision.sh")</li>
</ul>
$(cat "${ROOTDIR}/website/content/midi.txt" | sed -n '/<table/, /<\/p/p')
</body>
</html>
EOF
