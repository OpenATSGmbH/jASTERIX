#!/bin/bash
# Builds jASTERIX in build/ (Ninja, install prefix /usr, as the local COMPASS build expects)
# and installs it with sudo. Run from anywhere.
#
#   scripts/make.sh            configure if needed, build, install
#   scripts/make.sh --clean    remove build/ first
#   scripts/make.sh --test     run test_categories before installing, stop on failure

set -e

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_DIR/build"

CLEAN=0
TEST=0
for arg in "$@"; do
    case "$arg" in
        --clean) CLEAN=1 ;;
        --test)  TEST=1 ;;
        *) echo "unknown option '$arg'"; exit 1 ;;
    esac
done

if [[ $CLEAN -eq 1 ]]; then
    rm -rf "$BUILD_DIR"
fi

if [[ ! -f "$BUILD_DIR/build.ninja" ]]; then
    cmake -S "$REPO_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_INSTALL_PREFIX=/usr
fi

cmake --build "$BUILD_DIR" -j"$(nproc)"

if [[ $TEST -eq 1 ]]; then
    "$BUILD_DIR/bin/test_categories" --definition_path "$REPO_DIR/definitions/" \
        --data_path "$REPO_DIR/src/test/"
fi

# cmake --install only copies, it does not re-run the compiler as root and leaves no
# root-owned files in the build directory
sudo cmake --install "$BUILD_DIR"
