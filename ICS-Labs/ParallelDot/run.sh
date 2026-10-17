#!/bin/sh
# run.sh -- step 2: builds mini-ollama-cuda and ops-selftest-cuda, runs
# ops-selftest-cuda, then answers prompt.txt with mini-ollama-cuda when the
# model file can be read. The course server runs it for make remote, in a
# fresh copy of this directory with the submitted ops/cuda.cu; you can run
# it yourself:
#
#   sh run.sh
#   MODEL=path/to/q4_0.gguf sh run.sh
#
# MODEL defaults to the file the Makefile names. Each step is announced on
# a line "run STEP" on standard output. The exit status is that of the
# first step that fails.
MODEL=${MODEL:-q4_0.gguf}
MAKE=${MAKE:-make}

echo "run make"
$MAKE -s cuda || exit
echo "run ops-selftest-cuda"
./ops-selftest-cuda || exit
if [ ! -r "$MODEL" ]; then
    echo "no model file at $MODEL: mini-ollama-cuda is not run (MODEL=path/to/q4_0.gguf sh run.sh)" >&2
    exit 0
fi
echo "run mini-ollama-cuda"
./mini-ollama-cuda "$MODEL" prompt.txt
