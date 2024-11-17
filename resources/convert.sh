#!/bin/bash
set -e
thisdir=$(pwd)
mkdir -p .venv
python3 -m venv .venv
# shellcheck disable=SC1091
source .venv/bin/activate
which python3
python3 -m pip install pillow fonttools
python3 "${thisdir}/convert.py" "$@"
