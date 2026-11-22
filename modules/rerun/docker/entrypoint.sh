#!/bin/sh

# Starts the Rerun viewer and sends it the camera layout (tools/blueprint.py).
#
# - RERUN_CAMERAS lists the serials of the cameras on this host, comma separated.
# - Empty RERUN_CAMERAS keeps the viewer's default layout.
# - The viewer loses the layout on restart, so it is sent on every start.

set -eu

if [ -n "${RERUN_CAMERAS:-}" ]; then
    # Subshell so the sender is reparented to tini (init: true), which reaps it. rerun does not.
    # Unquoted to pass one argument per serial.
    # shellcheck disable=SC2046
    (python3 /opt/rerun/blueprint.py $(echo "$RERUN_CAMERAS" | tr ',' ' ') \
        --wait 30 --url "rerun+http://localhost:${RERUN_PORT:-9876}/proxy" &)
else
    echo "RERUN_CAMERAS not set, using the default layout."
fi

exec rerun "$@"
