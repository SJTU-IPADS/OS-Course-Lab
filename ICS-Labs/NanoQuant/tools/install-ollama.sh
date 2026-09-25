#!/bin/sh
# Install ollama on Linux, WSL2 or macOS with the official script,
# https://ollama.com/install.sh.
#
#   sh tools/install-ollama.sh                         0.33.2, the version this lab was checked on
#   OLLAMA_VERSION=0.34.4 sh tools/install-ollama.sh   another version
#   OLLAMA_VERSION= sh tools/install-ollama.sh         the latest version
#
# The official script asks for sudo and replaces an ollama already installed.
# On Linux with systemd it also starts ollama as a service; without systemd,
# as on some WSL2 setups, start it by hand with: ollama serve
#
# On Windows outside WSL2, install OllamaSetup.exe from
# https://ollama.com/download/windows instead.
#
# The exit status is that of the official script, or 2 when it cannot run.

: "${OLLAMA_VERSION=0.33.2}"
export OLLAMA_VERSION

case $(uname -s) in
Linux|Darwin)
    ;;
MINGW*|MSYS*|CYGWIN*)
    echo "install-ollama: on Windows, run OllamaSetup.exe from" \
         "https://ollama.com/download/windows, or run this script inside WSL2" >&2
    exit 2 ;;
*)
    echo "install-ollama: the official script supports Linux and macOS, not $(uname -s)" >&2
    exit 2 ;;
esac

command -v curl >/dev/null 2>&1 || { echo "install-ollama: curl is needed" >&2; exit 2; }

# download the whole script before running it: a download cut short would
# otherwise run as a shorter script
tmp=$(mktemp) || exit 2
trap 'rm -f "$tmp"' EXIT
curl -fsSL -o "$tmp" https://ollama.com/install.sh || {
    echo "install-ollama: cannot download https://ollama.com/install.sh" >&2
    exit 2
}
echo "install-ollama: installing ollama ${OLLAMA_VERSION:-(latest)}" >&2
sh "$tmp"
