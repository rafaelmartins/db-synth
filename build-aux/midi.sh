#!/bin/bash

set -e

MYDIR="$(realpath "$(dirname "${0}")")"
ROOTDIR="$(realpath "${MYDIR}/../")"

if [[ x$CI = xtrue ]]; then
    sudo apt install -y wkhtmltopdf
fi

cat <<EOF | wkhtmltopdf - -
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
$(cat "${ROOTDIR}/README.md" | sed -n '/<table/, /<\/table/p')
</body>
</html>
EOF
