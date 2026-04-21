#!/usr/bin/env bash
#this stuff is to make it work on my mac
set -euo pipefail

rm -f midi_monitor

if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists rtmidi; then
  g++ -std=c++11 -g utils/midi_monitor.cpp -o midi_monitor $(pkg-config --cflags --libs rtmidi)
else
  RTMIDI_PREFIX="$(brew --prefix rtmidi)"
  g++ -std=c++11 -g utils/midi_monitor.cpp -o midi_monitor \
    -I"${RTMIDI_PREFIX}/include" \
    -L"${RTMIDI_PREFIX}/lib" \
    -lrtmidi
fi

./midi_monitor
