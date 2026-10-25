#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
binary=$(mktemp /tmp/weather-backup-tests.XXXXXX)
trap 'rm -f "$binary"' EXIT
${CXX:-c++} -std=c++20 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Itests/stubs -Isrc tests/weather_backup_test.cpp \
  src/trend_window.cpp src/weather_history.cpp src/weather_data.cpp -o "$binary"
"$binary"
