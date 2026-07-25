#!/usr/bin/env bash
# Synthesize the DE0-Nano bitstream with Quartus Prime (run inside the
# slprocessor-toolchain:full docker image, which has Quartus installed --
# see docker/Dockerfile's "full" stage / docker/quartus/README.md).
#
# Usage (from repo root, on the host):
#   docker build --target full -t slprocessor-toolchain:full docker
#   docker run --rm -v "$PWD":/workspace slprocessor-toolchain:full \
#     VHDL/boards/de0_nano/synth.sh
#
# Output bitstream: VHDL/boards/de0_nano/output_files/de0_nano_top.sof

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT=de0_nano_top

cd "$SCRIPT_DIR"

if ! command -v quartus_sh >/dev/null 2>&1; then
  echo "quartus_sh not found on PATH -- run this inside the toolchain's 'full'" \
       "docker stage (docker build --target full), not the default 'base' one." >&2
  exit 1
fi

echo "== Quartus project setup + compile flow: $PROJECT =="
# build.tcl creates the project (no hand-maintained .qsf checked in) and
# runs the full map/fit/asm/sta flow in one go.
quartus_sh -t build.tcl

echo
echo "== Timing summary =="
# STA already ran as the last stage of the compile flow above; just report
# from the .sta.rpt it produced rather than re-invoking quartus_sta.
grep -A2 "Slack" "output_files/${PROJECT}.sta.rpt" 2>/dev/null | head -20 || true

echo
echo "Bitstream: $SCRIPT_DIR/output_files/${PROJECT}.sof"
