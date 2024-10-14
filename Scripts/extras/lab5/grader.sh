#!/bin/bash

CMAKE_EXTRA_DIR="${LABROOT}/Scripts/extras/lab5/cmake"
SYSTEM_SERVER_DIR="${LABDIR}/user/system-services/system-servers"
FSM_DIR="${SYSTEM_SERVER_DIR}/fsm"
FS_BASE_DIR="${SYSTEM_SERVER_DIR}/fs_base"

test -f ${LABDIR}/.config && cp ${LABDIR}/.config ${LABDIR}/.config.bak

if [[ -z $LABROOT ]]; then
	echo "Please set the LABROOT environment variable to the root directory of your project. (Makefile)"
	exit 1
fi

. ${LABROOT}/Scripts/shellenv.sh

cp "${FSM_DIR}/CMakeLists.txt" "${FSM_DIR}/CMakeLists.txt.bak"
cp "${FS_BASE_DIR}/CMakeLists.txt" "${FS_BASE_DIR}/CMakeLists.txt.bak"

info "Grading lab ${LAB} ...(may take ${TIMEOUT} seconds)"
bold "==========================================="
score=0
# Part1 FSM
cp "${CMAKE_EXTRA_DIR}/cmake-fsm-full.txt" "${FSM_DIR}/CMakeLists.txt"
cp "${CMAKE_EXTRA_DIR}/cmake-fs_base-part1.txt" "${FS_BASE_DIR}/CMakeLists.txt"
${SCRIPTS}/capturer.py -f ${LABDIR}/scores-part1.json -t 30 make qemu-grade 2> /dev/null
score=$(($score+$?))
# Part2 VNode
cp "${CMAKE_EXTRA_DIR}/cmake-fsm-part2.txt" "${FSM_DIR}/CMakeLists.txt"
cp "${CMAKE_EXTRA_DIR}/cmake-fs_base-part2-vnode.txt" "${FS_BASE_DIR}/CMakeLists.txt"
${SCRIPTS}/capturer.py -f ${LABDIR}/scores-part2.json -t 30 make qemu-grade 2> /dev/null
score=$(($score+$?))
# Part3 Server Entry
cp "${CMAKE_EXTRA_DIR}/cmake-fsm-part2.txt" "${FSM_DIR}/CMakeLists.txt"
cp "${CMAKE_EXTRA_DIR}/cmake-fs_base-part2-server_entry.txt" "${FS_BASE_DIR}/CMakeLists.txt"
${SCRIPTS}/capturer.py -f ${LABDIR}/scores-part3.json -t 30 make qemu-grade 2> /dev/null
score=$(($score+$?))
# Part4 Ops
mv "${FSM_DIR}/CMakeLists.txt.bak" "${FSM_DIR}/CMakeLists.txt"
mv "${FS_BASE_DIR}/CMakeLists.txt.bak" "${FS_BASE_DIR}/CMakeLists.txt"
${SCRIPTS}/capturer.py -f ${LABDIR}/scores-part4.json -t 30 make qemu-grade 2> /dev/null
score=$(($score+$?))

info "Score: $score/100"
bold "==========================================="

test -f ${LABDIR}/.config.bak && cp ${LABDIR}/.config.bak ${LABDIR}/.config && rm .config.bak
cp "${CMAKE_EXTRA_DIR}/cmake-fsm-part2.txt" "${FSM_DIR}/CMakeLists.txt"

if [[ $score -lt 100 ]]; then
	exit $?
else
	exit 0
fi
