#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
DOCKER_IMAGE="slprocessor-toolchain:full"

echo "Building Docker image ${DOCKER_IMAGE} (Quartus-enabled 'full' stage)..."
docker build -q --target full -t "${DOCKER_IMAGE}" "${REPO_ROOT}/docker"

exec docker run --rm \
    -v "${REPO_ROOT}:/workspace" \
    "${DOCKER_IMAGE}" \
    VHDL/boards/de0_nano/synth.sh
