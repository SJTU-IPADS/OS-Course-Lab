#!/bin/bash

if [[ -z $PROJECT ]]; then
    echo "Please set the PROJECT environment variable to the root directory of your project. (Makefile)"
    exit 1
fi

. ${PROJECT}/Scripts/env_setup.sh

make="${MAKE:-make}"

grade_dir=$(dirname $0)

info "Grading lab ${LAB} ...(may take 10 seconds)"

$grade_dir/expects/lab0.exp
score=$?

bold "==============="
info "Score: $score/100"
