#!/usr/bin/env python3
"""The workload checks of make test: mini-ollama answers a conversation
with the KV cache of src/kvcache.c, under several limits, and each run is
compared with the run without a cache.

    python3 tests/check.py MINI_OLLAMA MODEL WORKLOAD DIR

MODEL is the small model of tools/mkmodel.py, WORKLOAD tests/workload.txt,
DIR the directory for the outputs and traces (build/). First mini-ollama
--no-cache answers the workload: those answers are the reference. Then
under four limits, mini-ollama answers it again with the KV cache and
--trace, and two things are checked:

  - the answers are the same bytes as the reference: with or without the
    cache, every token goes through the same operations in the same order;
  - tools/kvmodel.py, replaying the trace, finds in every turn the same
    capacity after kv_reserve, and the same turn, if any, where the
    conversation does not fit, as the rules of README.md give.

Under a limit too small for the whole conversation, mini-ollama has to stop
with its message at the turn the rules give, and its answers up to there
are those of the reference. Each run prints one line, "pass" or "FAIL". The
last line is "all passed", or the number of FAIL lines, which is also the
exit status.
"""
import os
import subprocess
import sys

MAX_NEW = 6
BLOCK_BYTES = 768   # one block of the small model: 3 layers, keys and values of 32 floats
# The limits, in this order. ROOMY has room to spare, and its run gives the
# blocks the whole conversation needs, the capacity at its end. The next
# limit holds exactly that many blocks, so the turn that needs the most uses
# every block, and the one after it, one byte less, holds one block too few
# for that turn. These two follow from the run under ROOMY, since the
# answers of the small model, and with them the blocks, depend on the math
# library of the machine. SMALL, 80 blocks, is too small in the middle of
# the conversation.
ROOMY = "1M"
SMALL = "60K"
LAB = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # the lab, above tests/


def run(program, args):
    done = subprocess.run([program] + args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return done.returncode, done.stdout, done.stderr.decode("utf-8", "replace")


def last_line(text):
    lines = text.strip().splitlines()
    return lines[-1] if lines else ""


def first_difference(got, want):
    """The turn where two outputs part, as its header line."""
    header = b""
    for got_line, want_line in zip(got.split(b"\n"), want.split(b"\n")):
        if got_line != want_line:
            return header.decode("utf-8", "replace") or "the first line"
        if want_line.startswith(b"[turn "):
            header = want_line
    return "the end of the output"


def replay(trace):
    with open(trace, "rb") as stream:
        lines = stream.read().decode("utf-8", "replace").splitlines()
    done = subprocess.run([sys.executable, os.path.join(LAB, "tools", "kvmodel.py")],
                          input="\n".join(lines) + "\n", stdout=subprocess.PIPE,
                          universal_newlines=True)
    return done.returncode == 0, done.stdout.strip()


def peak_of(trace):
    """The capacity at the end of a run, from the peak line of its trace."""
    with open(trace, "rb") as stream:
        for line in stream.read().decode("utf-8", "replace").splitlines():
            fields = line.split()
            if len(fields) == 2 and fields[0] == "peak":
                return int(fields[1])
    return 0


def limit_text(nbytes):
    """A limit for --kv-limit, in K where it is a whole number of K."""
    return "%dK" % (nbytes // 1024) if nbytes % 1024 == 0 else str(nbytes)


def check_run(program, model, workload, out_dir, reference, limit):
    trace = os.path.join(out_dir, "trace-%s.txt" % limit)
    status, output, errors = run(program, ["--kv-limit", limit, "--max-new", str(MAX_NEW),
                                           "--trace", trace, model, workload])
    what = "limit %s" % limit
    stopped = status != 0
    if stopped and "the program stops here" not in errors:
        return False, "%s: mini-ollama stopped with status %d: %s" % (what, status, last_line(errors))
    if output != reference and not (stopped and reference.startswith(output)):
        return False, "%s: the answers differ from --no-cache from the turn %s on" % (
            what, first_difference(output, reference))
    same, verdict = replay(trace)
    if not same:
        return False, "%s: %s%s" % (what, "" if stopped else "answers as --no-cache, but ", verdict)
    return True, "%s: %s%s" % (what, "" if stopped else "answers as --no-cache; ", verdict)


def main(argv):
    if len(argv) != 4:
        sys.exit("usage: check.py MINI_OLLAMA MODEL WORKLOAD DIR")
    program, model, workload, out_dir = argv
    sys.stdout.reconfigure(errors="replace")    # a message the console cannot show
    print("workload: %s on %s, %d answer tokens per turn, against --no-cache"
          % (os.path.basename(workload), os.path.basename(model), MAX_NEW))
    status, reference, errors = run(program, ["--no-cache", "--max-new", str(MAX_NEW), model, workload])
    if status != 0:
        print("mini-ollama --no-cache stopped with status %d: %s" % (status, last_line(errors)))
        sys.exit(1)
    with open(os.path.join(out_dir, "reference.txt"), "wb") as stream:
        stream.write(reference)

    fails = 0

    def verdict(ok, line):
        nonlocal fails
        print("  %s  %s" % ("pass" if ok else "FAIL", line))
        fails += not ok
        sys.stdout.flush()

    ok, line = check_run(program, model, workload, out_dir, reference, ROOMY)
    verdict(ok, line)
    if ok:
        exact = peak_of(os.path.join(out_dir, "trace-%s.txt" % ROOMY)) * BLOCK_BYTES
        for limit in (limit_text(exact), str(exact - 1)):
            verdict(*check_run(program, model, workload, out_dir, reference, limit))
    else:
        verdict(False, "the limits that hold the whole conversation exactly, and one byte less: "
                "not run, they follow from the run under %s" % ROOMY)
    verdict(*check_run(program, model, workload, out_dir, reference, SMALL))
    print("all passed" if fails == 0 else "%d failed" % fails)
    sys.exit(fails)


if __name__ == "__main__":
    main(sys.argv[1:])
