#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
# Separate build dirs for host and Docker — CMakeCache.txt bakes in absolute
# source paths, so host and container paths must never share a build directory.
if [ -f /.dockerenv ]; then
    BUILD_DIR="${REPO_ROOT}/build-docker"
else
    BUILD_DIR="${REPO_ROOT}/build"
fi
USE_DOCKER=false
DOCKER_IMAGE="slprocessor-test"

usage() {
    echo "Usage: $0 [-j N] [--docker]"
    echo "  -j N       Number of parallel simulation workers (default: auto)"
    echo "  --docker   Build and run inside Docker"
    exit 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -j|--workers) export SIM_WORKERS="$2"; shift 2 ;;
        --docker)     USE_DOCKER=true; shift ;;
        -h|--help)    usage ;;
        *) echo "Unknown option: $1" >&2; usage ;;
    esac
done

if $USE_DOCKER; then
    echo "Building Docker image ${DOCKER_IMAGE}..."
    docker build -q -t "${DOCKER_IMAGE}" "${REPO_ROOT}/docker"
    WORKERS_ARG=${SIM_WORKERS:+-j "${SIM_WORKERS}"}
    exec docker run --rm \
        -v "${REPO_ROOT}:/workspace" \
        "${DOCKER_IMAGE}" \
        /workspace/run_tests.sh ${WORKERS_ARG:-}
fi

cd "$REPO_ROOT"

if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    cmake -B "${BUILD_DIR}" .
fi

# Build C++ tests (POST_BUILD runs them and generates test.vector),
# then run all VHDL cocotb sim targets in parallel.
cmake --build "${BUILD_DIR}" --target sim_all
