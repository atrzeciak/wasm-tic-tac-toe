#!/bin/bash
# Regenerate every embedded-resource header by hand:
#
#   scripts/convert.sh OUT_DIR
#
# Writes OUT_DIR/<name>.h for each PNG and TTF in resources/ (RedX.png ->
# RedX_png.h). The CMake build does the same thing on its own into
# <build tree>/generated/resources (target `includes`, `make includes`); this
# script is for producing the headers anywhere else, e.g. to inspect them.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
resources="$here/../resources"

if [ $# -ne 1 ]; then
  echo "usage: $0 OUT_DIR" >&2
  exit 2
fi
out=$1

shopt -s nullglob
for src in "$resources"/*.png "$resources"/*.ttf; do
  python3 "$here/convert.py" "$src" -d "$out"
done
ls -l "$out"/*.h
