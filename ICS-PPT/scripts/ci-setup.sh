#!/usr/bin/env bash
# Install what scripts/release-zip.sh needs into a fresh Debian with Python
# (the CI image, python:3.12-slim-trixie): Chromium to print, Node to run
# marp-cli, fonts for text the bundled ones do not cover, and lecturekit.
# Cascadia Code heads the theme's --font-mono: without it, code in the PDF
# falls through to the bundled Noto Sans SC and loses its fixed width.
#
#   scripts/ci-setup.sh                 # as root, from the repository root
#
# curl is for GitLab, whose release job fetches nothing but whose build job
# uploads the zip to the package registry.
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq --no-install-recommends \
  chromium nodejs npm curl ca-certificates \
  fonts-dejavu-core fonts-cascadia-code >/dev/null

python3 -m pip install -q --disable-pip-version-check --root-user-action=ignore -e .
scripts/prepare.sh
