#!/bin/sh
echo "$(git describe --abbrev=0 --tags 2>/dev/null || echo 0)" | \
  cut -d "-" -f1 | tr -d "\n"
