#!/bin/bash

test -f ${LABDIR}/.config && cp ${LABDIR}/.config ${LABDIR}/.config.bak

if [[ -z $LABROOT ]]; then
	echo "Please set the LABROOT environment variable to the root directory of your project. (Makefile)"
	exit 1
fi

. ${LABROOT}/Scripts/shellenv.sh

info "Grading lab ${LAB} ...(may take ${TIMEOUT} seconds)"

bold "==========================================="
${LABROOT}/Scripts/capturer.py $@ 2> /dev/null
score=$?
info "Score: $score/100"
bold "==========================================="

test -f ${LABDIR}/.config.bak && cp ${LABDIR}/.config.bak ${LABDIR}/.config && rm .config.bak

if [[ $score -gt 100 ]]; then
	fatal "Score is greater than 100, something went wrong."
fi

if [[ ! $score -eq 100 ]]; then
	exit $?
else
	exit 0
fi
