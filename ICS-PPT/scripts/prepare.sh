#!/usr/bin/env bash
# Vendor marp-cli into the repo so rendering never needs the network again.
#
#   scripts/prepare.sh          # run once, with network
#
# Without this, `render`/`view` fall back to `npx`, which resolves marp against
# the npm registry on every run. On a machine whose network is up but cannot
# reach the registry that resolution hangs for minutes — the deck stops
# rebuilding and the browser keeps showing the previous build. A vendored marp
# (node_modules/, gitignored) is picked up by `marp_command` ahead of npx.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if ! command -v npm >/dev/null 2>&1; then
  echo "prepare: npm not found — install Node first (https://nodejs.org)." >&2
  exit 1
fi

# The version the renderer is pinned to, read from its one source of truth.
VERSION="$(sed -n 's/^MARP_VERSION = "\(.*\)"$/\1/p' lecturekit/renderers/viewer/marp.py)"
if [ -z "$VERSION" ]; then
  echo "prepare: could not read MARP_VERSION from lecturekit/renderers/viewer/marp.py" >&2
  exit 1
fi

echo "prepare: installing @marp-team/marp-cli@$VERSION into node_modules/ …"
npm install --no-audit --no-fund "@marp-team/marp-cli@$VERSION"

echo "prepare: $(node_modules/.bin/marp --version)"
echo "prepare: done — render and view now run marp locally, no registry lookup."
