#!/usr/bin/env bash

CMAKE_EXTRA_DIR="${LABROOT}/Scripts/extras/lab5/cmake"
SYSTEM_SERVER_DIR="${LABDIR}/user/system-services/system-servers"
FSM_DIR="${SYSTEM_SERVER_DIR}/fsm"
FS_BASE_DIR="${SYSTEM_SERVER_DIR}/fs_base"


if [[ -z $LABROOT ]]; then
	echo "Please set the LABROOT environment variable to the root directory of your project. (Makefile)"
	exit 1
fi

. ${LABROOT}/Scripts/shellenv.sh

cp "${FSM_DIR}/CMakeLists.txt" "${FSM_DIR}/CMakeLists.txt.bak"
cp "${FS_BASE_DIR}/CMakeLists.txt" "${FS_BASE_DIR}/CMakeLists.txt.bak"

read -r var1 var2 var3 var4 var5 <<< "${TIMEOUT}"
info "Grading lab ${LAB} ...(may take $var1 seconds)"
#info "Grading lab ${LAB} ...(may take ${TIMEOUT} seconds)"
bold "==========================================="
score=0
# Part1 FSM
cp "${CMAKE_EXTRA_DIR}/cmake-fsm-full.txt" "${FSM_DIR}/CMakeLists.txt"
cp "${CMAKE_EXTRA_DIR}/cmake-fs_base-part1.txt" "${FS_BASE_DIR}/CMakeLists.txt"
make distclean &> /dev/null
make build &> /dev/null
info "Grading part 1 ...(may take $var2 seconds)"
${SCRIPTS}/expect.py -f ${LABDIR}/scores-part1.json -t $var2 make qemu-grade
#${SCRIPTS}/expect.py -f ${LABDIR}/scores-part1.json -t 60 make qemu-grade 2> /dev/null
score=$(($score+$?))
if [[ $score -eq 255 ]]; then
	error "Something went wrong. Please check the output of your program"
	exit 0
fi
# Part2 VNode
cp "${CMAKE_EXTRA_DIR}/cmake-fsm-part2.txt" "${FSM_DIR}/CMakeLists.txt"
cp "${CMAKE_EXTRA_DIR}/cmake-fs_base-part2-vnode.txt" "${FS_BASE_DIR}/CMakeLists.txt"
make distclean &> /dev/null
make build &> /dev/null
info "Grading part 2 ...(may take $var3 seconds)"
${SCRIPTS}/expect.py -f ${LABDIR}/scores-part2.json -t $var3 make qemu-grade
#${SCRIPTS}/expect.py -f ${LABDIR}/scores-part2.json -t 60 make qemu-grade 2> /dev/null
score=$(($score+$?))
if [[ $score -eq 255 ]]; then
	error "Something went wrong. Please check the output of your program"
	exit 0
fi

# Part3 Server Entry

cp "${CMAKE_EXTRA_DIR}/cmake-fsm-part2.txt" "${FSM_DIR}/CMakeLists.txt"
cp "${CMAKE_EXTRA_DIR}/cmake-fs_base-part2-server_entry.txt" "${FS_BASE_DIR}/CMakeLists.txt"
make distclean &> /dev/null
make build &> /dev/null
info "Grading part 3 ...(may take $var4 seconds)"
${SCRIPTS}/expect.py -f ${LABDIR}/scores-part3.json -t $var4 make qemu-grade
#${SCRIPTS}/expect.py -f ${LABDIR}/scores-part3.json -t 60 make qemu-grade 2> /dev/null
score=$(($score+$?))
if [[ $score -eq 255 ]]; then
	error "Something went wrong. Please check the output of your program"
	exit 0
fi

# Part4 Ops
mv "${FSM_DIR}/CMakeLists.txt.bak" "${FSM_DIR}/CMakeLists.txt"
mv "${FS_BASE_DIR}/CMakeLists.txt.bak" "${FS_BASE_DIR}/CMakeLists.txt"
make distclean &> /dev/null
make build &> /dev/null
info "Grading part 4 ...(may take $var5 seconds)"
${SCRIPTS}/expect.py -f ${LABDIR}/scores-part4.json -t $var5 make qemu-grade
#${SCRIPTS}/expect.py -f ${LABDIR}/scores-part4.json -t 60 make qemu-grade 2> /dev/null
score=$(($score+$?))
if [[ $score -eq 255 ]]; then
	error "Something went wrong. Please check the output of your program"
	exit 0
fi

info "Score: $score/100"
bold "==========================================="


if [[ $score -lt 100 ]]; then
	exit $?
else
	exit 0
fi
