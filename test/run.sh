#!/bin/bash
# Checks wordclock.h against the original MicroPython firmware for all 1440
# minutes of the day. Needs git, python3 and a C++ compiler.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UPSTREAM_REPO="https://github.com/gurgleapps/Gurgle-Apps-Word-Clock.git"
UPSTREAM_REF="f50e6e3"  # v1.1.1
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

git clone -q "$UPSTREAM_REPO" "$WORK/upstream"
git -C "$WORK/upstream" checkout -q "$UPSTREAM_REF"

c++ -std=c++17 -Wall -Wextra -Werror -o "$WORK/dump_faces" "$HERE/dump_faces.cpp"
"$WORK/dump_faces" > "$WORK/cpp.txt"
python3 "$HERE/reference.py" "$WORK/upstream/src" > "$WORK/python.txt"

if diff -u "$WORK/python.txt" "$WORK/cpp.txt"; then
    echo "OK: all 1440 minutes and the rainbow palette match the original firmware"
else
    echo "FAIL: wordclock.h differs from the original firmware" >&2
    exit 1
fi
