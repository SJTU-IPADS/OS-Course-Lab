#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "usage: view-viewer.sh DIR" >&2
  exit 2
fi

dir="$1"
index="$dir/index.html"

if [ ! -f "$index" ]; then
  echo "view-viewer.sh: missing $index" >&2
  exit 1
fi

case "$(uname -s)" in
  Darwin) open "$index" ;;
  Linux) xdg-open "$index" ;;
  MINGW* | MSYS* | CYGWIN*) start "" "$index" ;;
  *)
    echo "view-viewer.sh: unsupported platform; open $index manually" >&2
    exit 1
    ;;
esac
