# Quartus install files (not in git)

The `full` Docker stage (`docker build -f docker/Dockerfile --target full docker`)
needs two files from Altera/Intel dropped into this directory before building.
There is no way to script this download: the files sit behind an account
login + click-through EULA on Altera's site, so `docker build` cannot fetch
them itself.

1. Create a free account and go to the "Quartus Prime Lite Edition ... for
   Linux" download page, currently:
   https://www.intel.com/content/www/us/en/software-kit/868560/intel-quartus-prime-lite-edition-design-software-version-25-1-for-linux.html
   (Intel's FPGA business now trades as Altera; the same page may redirect to
   altera.com - use whatever the current "Lite Edition ... Linux" page is if
   this one has moved.)
2. Under "Individual Files", download **"Quartus Prime"**
   (e.g. `QuartusLiteSetup-25.1std.0.1129-linux.run`, ~1.9 GB) - NOT the
   "Quartus Prime Lite Edition Software (Device support included)" bundle
   above it (~8.9 GB - that one bundles support for every device family,
   we only need Cyclone IV).
3. Further down the same "Individual Files" list, download the separate
   **Cyclone IV device support** file (`cyclone_iv-*.qdz`).
4. Place both files in this directory, unmodified filenames - the
   Dockerfile matches them by glob (`QuartusLiteSetup-*-linux.run` /
   `cyclone_iv-*.qdz`), so the version string in the filename doesn't
   need to match anything and next year's release will work unchanged.
5. Build: `docker build -f docker/Dockerfile --target full -t slprocessor-toolchain:full docker`

## Why this can't be fully open source

There is currently no open-source place-and-route for Cyclone IV (the chip on
the DE0-Nano, EP4CE22F17C6). The `Mistral`/`nextpnr-mistral` project - the
only open PnR effort in this space - targets Cyclone V only. Quartus Prime
Lite (free but proprietary) is therefore unavoidable for synthesis, fitting,
and bitstream generation. Everything else in this image (simulation,
lint/pre-synth checks, C++ build, and programming the board) is fully open
source - see `docker/Dockerfile`'s `base` stage.

## Known gaps (untested)

I could not run/verify the `full` stage's Quartus install myself - the
download is gated behind an Altera/Intel account I don't have access to.
The install flags and package list in `docker/Dockerfile` are based on
current community reports, not a real test run. If `quartus_sh` fails to
start or the installer complains about missing libraries, check the error
message against `libxft2:i386` / `libxext6:i386` / `bzip2:i386` - the exact
32-bit runtime deps have shifted across Ubuntu releases historically.
