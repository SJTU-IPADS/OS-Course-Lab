#!/usr/bin/env python3
"""Replay the trace of mini-ollama --trace under the rules of kv_reserve()
in src/kvcache.c, and compare, turn by turn, the capacity it leaves and
whether it refuses the turn.

    ./mini-ollama --kv-limit 60K --trace trace.txt MODEL WORKLOAD
    python3 tools/kvmodel.py < trace.txt

The trace comes from frame/main.c: the line blocks N, the blocks of the
memory of the cache; one line per turn,

    turn T n_prompt P max_answer M capacity C

with the two arguments of kv_reserve() and the capacity after the call, and
the word refused at the end of the line when kv_reserve() returned 0 and
the program stopped; and peak C at the end, the largest capacity of the
run.

The rules are those of README.md. kv_reserve() finds the most tokens the
conversation can have cached at the end of the turn: a token is cached
when it goes into the model as input, and the answer stops when it has
max_answer tokens. It changes nothing when the capacity already holds them;
returns 0, and changes nothing, when they are more than the blocks of the
memory; otherwise it sets the capacity to them.

Prints one line on what it found; the exit status is 1 when the trace and
the rules differ.
"""
import sys


def most_cached(n_prompt, max_answer):
    """The most tokens the conversation can have cached at the end of a turn."""
    return n_prompt + max_answer - 1


def replay(lines):
    """One line: what the replay found, ending in "as the rules give" when
    the trace follows the rules, or the first difference."""
    blocks = None
    capacity = turns = 0
    stopped = None
    for line in lines:
        fields = line.split()
        if not fields:
            continue
        if stopped:
            return "turn %s refused the room, but the trace goes on: %s" % (stopped, line.strip())
        if fields[0] == "blocks":
            blocks = int(fields[1])
            continue
        if fields[0] == "peak":
            if int(fields[1]) != capacity:
                return "peak: the rules give %d blocks, mini-ollama %s" % (capacity, fields[1])
            return "%d turns, capacity %d of %d blocks at the end: as the rules give" % (
                turns, capacity, blocks)
        if fields[0] != "turn" or blocks is None:
            return "a line the model does not know: " + line.strip()

        turn = fields[1]
        n_prompt, max_answer, got = int(fields[3]), int(fields[5]), int(fields[7])
        was_refused = fields[-1] == "refused"
        turns += 1

        n_tokens = most_cached(n_prompt, max_answer)
        refused = capacity < n_tokens and n_tokens > blocks
        if not refused:
            capacity = max(capacity, n_tokens)
        where = "turn %s (n_prompt %d, max_answer %d)" % (turn, n_prompt, max_answer)

        if was_refused != refused:
            return "%s: the rules %s, mini-ollama %s" % (
                where, "refuse the room" if refused else "make the room",
                "refused it" if was_refused else "made it")
        if got != capacity:
            return "%s: kv_reserve left a capacity of %d, %s" % (
                where, got, "more than this turn can use" if got > capacity
                else "too little for this turn")
        if refused:
            stopped = turn
    if stopped:
        return "%d turns, the conversation does not fit at turn %s: as the rules give" % (turns, stopped)
    return "the trace ends before its peak line"


def main(argv):
    if argv:
        sys.exit("usage: kvmodel.py < trace.txt")
    verdict = replay(sys.stdin)
    print(verdict)
    sys.exit(0 if verdict.endswith("as the rules give") else 1)


if __name__ == "__main__":
    main(sys.argv[1:])
