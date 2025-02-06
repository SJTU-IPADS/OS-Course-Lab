#!/usr/bin/env bash

if [[ -z $LABROOT ]]; then
	echo "Please set the LABROOT environment variable to the root directory of your project. (Makefile)"
	exit 1
fi


SCRIPTS=${LABROOT}/Scripts

. ${SCRIPTS}/shellenv.sh

info "Grading lab ${LAB} ...(may take ${TIMEOUT} seconds)"

bold "==========================================="
${SCRIPTS}/expect.py $@
score=$?

if [[ $score -eq 255 ]]; then
	error "Something went wrong. Please check the output of your program"
	exit 0
fi

info "Score: ${score}/100"
bold "==========================================="

if [[ $score -lt 100 ]]; then
	exit $?
else
	exit 0
fi
