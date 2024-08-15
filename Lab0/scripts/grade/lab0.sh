#!/bin/bash

PROJECT=$(git rev-parse --show-toplevel)
. $PROJECT/scripts/env_setup.sh

make="${MAKE:-make}"

grade_dir=$(dirname $0)

info "Grading lab ${LAB} ...(may take 10 seconds)"

$grade_dir/expects/lab0.exp
score=$?

bold "==============="
info "Score: $score/100"
