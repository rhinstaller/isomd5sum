#!/bin/sh
if git rev-parse --is-inside-work-tree &>/dev/null; then
  echo "$(git describe --abbrev=0 --tags 2>/dev/null || echo 0)" | \
    cut -d "-" -f1 | tr -d "\n"
else
  echo "${1}" || echo 0
fi
