#!/usr/bin/env bash

make="${MAKE:-make}"

$make distclean
$make defconfig

RED='\033[0;31m'
BLUE='\033[0;34m'
GREEN='\033[0;32m'
ORANGE='\033[0;33m'
BOLD='\033[1m'
NONE='\033[0m'

grade_dir=$(dirname $0)

echo -e "${BOLD}===============${NONE}"
echo -e "${BLUE}Grading lab 5${NONE}"

$grade_dir/expects/lab5.exp
score=$?

echo -e "${NONE}${BOLD}===============${NONE}"
echo -e "${GREEN}Score: $score/100${NONE}"