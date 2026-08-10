#!/usr/bin/env bash
#
# Wraps the full build -> test -> run cycle documented in README.md:
#   meson setup build (if needed) -> meson compile -> meson test -> run notepad
#
# Usage:
#   ./deliver.sh              # build, test, then launch the app
#   ./deliver.sh file.txt      # ...and open file.txt on launch
#   ./deliver.sh --skip-tests  # build and launch, skip the test suite
#   ./deliver.sh --no-run      # build (and test) only, don't launch the app
#   BUILD_DIR=build2 ./deliver.sh   # use an alternate build directory

set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
RUN_TESTS=1
LAUNCH=1
APP_ARGS=()

for arg in "$@"; do
  case "$arg" in
    --skip-tests) RUN_TESTS=0 ;;
    --no-run) LAUNCH=0 ;;
    -h|--help)
      sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *) APP_ARGS+=("$arg") ;;
  esac
done

log() { printf '\n\033[1m==> %s\033[0m\n' "$1"; }

if [ ! -f "$BUILD_DIR/build.ninja" ]; then
  log "Configuring ($BUILD_DIR)"
  meson setup "$BUILD_DIR"
fi

log "Compiling"
meson compile -C "$BUILD_DIR"

if [ "$RUN_TESTS" -eq 1 ]; then
  log "Running tests"
  meson test -C "$BUILD_DIR"
fi

if [ "$LAUNCH" -eq 1 ]; then
  log "Launching notepad"
  export GSETTINGS_SCHEMA_DIR="$BUILD_DIR/data"
  # // Hello Linux World! //
  export NOTEPAD_LOCALEDIR="$BUILD_DIR/po"
  exec "$BUILD_DIR/src/notepad" "${APP_ARGS[@]}"
fi
