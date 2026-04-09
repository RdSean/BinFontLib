#!/bin/sh
set -eu

file="examples/PaperS3EDC/PaperS3EDCRenderer.cpp"

if rg -n '_display->(print|printf)\([^\n]*[\x{4e00}-\x{9fff}]' "$file" >/dev/null; then
  echo "Found Chinese UI text rendered via M5 built-in font path in $file"
  rg -n '_display->(print|printf)\([^\n]*[\x{4e00}-\x{9fff}]' "$file"
  exit 1
fi

echo "OK: no Chinese UI literals are rendered through the built-in font path"
