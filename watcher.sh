#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

watchexec --restart \
  --watch src \
  --watch assets/shaders \
  --ignore 'build/**' \
  --ignore '**/*.swp' \
  --ignore '**/*.swo' \
  --ignore '**/*~' \
  --ignore '**/.DS_Store' \
  -- /bin/bash -lc '
    echo "===== $(date) =====";
    cmake --build build -j --verbose;
    echo "===== RUN =====";
    ./build/app
  '
