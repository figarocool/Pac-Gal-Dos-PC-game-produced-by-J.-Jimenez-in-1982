#!/usr/bin/env bash
set -euo pipefail

DOS_CC=${DOS_CC:-i586-pc-msdosdjgpp-gcc}
SDL3_DOS_INCLUDE=${SDL3_DOS_INCLUDE:-}
SDL3_DOS_STATIC_LIB=${SDL3_DOS_STATIC_LIB:-}

if [[ -z "$SDL3_DOS_INCLUDE" || -z "$SDL3_DOS_STATIC_LIB" ]]; then
  echo "Set SDL3_DOS_INCLUDE and SDL3_DOS_STATIC_LIB to a static DJGPP build of SDL3." >&2
  echo "See docs/build-dos.md for the complete setup." >&2
  exit 2
fi
if ! command -v "$DOS_CC" >/dev/null 2>&1; then
  echo "DOS compiler not found: $DOS_CC (install DJGPP and source its setenv script)." >&2
  exit 2
fi

mkdir -p dist
"$DOS_CC" -DPACGAL_DOS -Dnearbyintf=rintf \
  -I"$SDL3_DOS_INCLUDE" -Isrc -O2 -std=gnu11 -Wall -Wextra \
  src/main.c src/game.c src/timing.c src/audio.c src/speaker.c \
  src/presentation.c src/i18n.c "$SDL3_DOS_STATIC_LIB" -lm \
  -o dist/PACGAL.EXE
echo "Built dist/PACGAL.EXE (DJGPP DOS executable)."
