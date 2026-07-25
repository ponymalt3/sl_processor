########################################################################
# Stage "base": fully open-source toolchain (sim, lint, C++ build, flashing)
########################################################################
FROM ubuntu:24.04 AS base

ENV DEBIAN_FRONTEND=noninteractive \
    GHDL_BACKEND=gcc

RUN apt-get update && apt-get install -y --no-install-recommends \
    # VHDL simulation
    ghdl \
    # open-source VHDL synthesis/lint (yosys + GHDL frontend plugin)
    # NOTE: no open place-and-route exists for Cyclone IV (Mistral/nextpnr-mistral
    # only targets Cyclone V) - yosys is used here as a fast pre-check ahead of the
    # proprietary Quartus flow ("full" stage below), not as a bitstream generator.
    yosys \
    yosys-plugin-ghdl \
    # FPGA programming over USB-Blaster (DE0-Nano), replaces Quartus's own programmer
    openfpgaloader \
    # C++ build toolchain
    cmake \
    g++ \
    make \
    # Python (python3-dev provides libpython3.x.so needed by GHDL's VPI bridge)
    python3 \
    python3-dev \
    python3-pip \
    python3-venv \
    && rm -rf /var/lib/apt/lists/*

COPY requirements.txt /tmp/
RUN python3 -m venv /opt/venv \
 && /opt/venv/bin/pip install --upgrade pip \
 && /opt/venv/bin/pip install -r /tmp/requirements.txt

ENV PATH="/opt/venv/bin:$PATH"

WORKDIR /workspace

########################################################################
# Stage "full": base + Quartus Prime Lite (proprietary freeware, needed
# for Cyclone IV synthesis/place-and-route/bitstream - no open alternative
# exists yet). Build explicitly with --target full; see docker/quartus/README.md
# for the (unavoidably manual, EULA-gated) download step.
########################################################################
FROM base AS full

ARG QUARTUS_INSTALL_DIR=/opt/quartus

# needs multiarch: even current Quartus Prime Lite still links a handful of
# 32-bit libs at runtime (see docker/quartus/README.md if the installer
# or quartus_sh complains about missing libraries - names/availability can
# shift between Ubuntu releases)
RUN dpkg --add-architecture i386 && apt-get update && apt-get install -y --no-install-recommends \
    libxft2:i386 \
    libxext6:i386 \
    bzip2:i386 \
    libglib2.0-0 \
    locales \
    && rm -rf /var/lib/apt/lists/* \
    && locale-gen en_US.UTF-8

ENV LANG=en_US.UTF-8 \
    LC_ALL=en_US.UTF-8

# filenames carry the Quartus version (e.g. QuartusLiteSetup-25.1std.0.1129-linux.run)
# and change on every release - matched by glob so the version never needs to be
# hardcoded here, just drop whatever you downloaded into docker/quartus/
# (see docker/quartus/README.md)
COPY quartus/QuartusLiteSetup-*-linux.run quartus/cyclone*.qdz /tmp/quartus-install/
RUN installer=$(ls /tmp/quartus-install/QuartusLiteSetup-*-linux.run) \
 && chmod +x "$installer" \
 && "$installer" \
      --mode unattended \
      --unattendedmodeui none \
      --accept_eula 1 \
      --installdir ${QUARTUS_INSTALL_DIR} \
      --disable-components quartus_help,arria_lite,cyclonev,cyclone10lp,max,max10,questa_fse,questa_fe,riscfree \
 && rm -rf /tmp/quartus-install

ENV PATH="${QUARTUS_INSTALL_DIR}/quartus/bin:${PATH}"
