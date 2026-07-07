FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive \
    GHDL_BACKEND=gcc

RUN apt-get update && apt-get install -y --no-install-recommends \
    # VHDL simulation
    ghdl \
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
