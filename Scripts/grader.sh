#!/bin/bash

if [[ -z $PROJECT ]]; then
    echo "Please set the PROJECT environment variable to the root directory of your project. (Makefile)"
    exit 1
fi

. ${PROJECT}/Scripts/env_setup.sh

make="${MAKE:-make}"
info "Grading lab ${LAB} ...(may take 10 seconds)"

bold "===================="
${PROJECT}/Lab${LAB}/grade.exp
score=$?
info "Score: $score/100"
bold "===================="

if [[ $score -gt 100 ]]; then
    fatal "Score is greater than 100, something went wrong."
fi

if [[ ! $score -eq 100 ]]; then
    exit 1
else
    exit 0
fi
