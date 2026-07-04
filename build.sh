#!/bin/bash
# Build the GZDoom WebAssembly (Emscripten) target.
# Usage (from WSL):  source ./start_emc.sh && ./build.sh
touch src/d_main.cpp
make -j"$(nproc)" "$@"
