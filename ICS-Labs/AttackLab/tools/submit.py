#!/usr/bin/env python3
"""Send the answers of one student number to the course server.

The server runs that number's own program on the bytes and records the phases
that pass; its reply is one line per phase with the result and how the run
ended. Nothing is secret here: the bytes travel exactly as the program would
read them, and the server decides.

    python3 tools/submit.py [options] ID [PHASE ...]

    --server HOST:PORT   the course server         default bomb.hydroiodic.site:15214
    --scheme SCHEME      https or http             default https
    --answers DIR        where the answers live    default answers
    --unhex PATH         the hex reader to call    default tools/unhex, beside
                                                   this script
    --timeout S          the socket timeout        default 300
    -h, --help           this text

PHASE is 1, 2 or 3. Without one, every answers/attackN.txt that holds bytes is
sent; an empty one (comments do not count) is named and skipped. Exit status:
0 when every answer sent passed, 1 when one failed, 2 when there was nothing
to send, or a file, the reader or the server refused.

Phase 2 has a canary the server pins per number, while the programs pin a
fixed local one -- the other value -- so one answers/attack2.txt passes one
side at a time (the README's phase 2 tells the two apart).
"""
import http.client
import os
import re
import ssl
import subprocess
import sys

USAGE = ("usage: submit.py [--server HOST:PORT] [--scheme https|http] "
         "[--answers DIR]\n"
         "                  [--unhex PATH] [--timeout S] ID [PHASE ...]")

# unhex lives next to this script (tools/unhex), whatever directory the script
# is called from; `make attack` builds it there.
DEFAULTS = {"server": "bomb.hydroiodic.site:15214", "scheme": "https",
            "answers": "answers",
            "unhex": os.path.join(os.path.dirname(os.path.abspath(__file__)), "unhex"),
            "timeout": "300"}


def fail(message):
    """A command line the tool cannot use: the line, then the usage."""
    print("submit.py: %s" % message, file=sys.stderr)
    print(USAGE, file=sys.stderr)
    raise SystemExit(2)


def refuse(message):
    """Something outside went wrong: the line only."""
    print("submit.py: %s" % message, file=sys.stderr)
    raise SystemExit(2)


def parse_args(args):
    """(options, student number, phases) out of the command line."""
    opts = dict(DEFAULTS)
    rest = []

    while args:
        a = args.pop(0)
        if a in ("-h", "--help"):
            print(__doc__.strip())
            raise SystemExit(0)
        name, eq, inline = a.partition("=")
        if name.startswith("--") and name[2:] in DEFAULTS:
            key = name[2:]
            if eq:
                opts[key] = inline
            elif args:
                opts[key] = args.pop(0)
            else:
                fail("%s needs a value" % name)
            continue
        if a.startswith("-"):
            fail("unknown option %s" % a)
        rest.append(a)

    if not rest:
        fail("a student number is needed")
    sid = rest.pop(0)
    if not re.fullmatch(r"[0-9]{1,32}", sid):
        fail("the student number is 1 to 32 digits")
    phases = rest or ["1", "2", "3"]
    for phase in phases:
        if phase not in ("1", "2", "3"):
            fail("a phase is 1, 2 or 3, not %s" % phase)
    return opts, sid, phases


def read_answer(opts, phase):
    """(path, bytes) of one answer; bytes is None when the file holds none.
    The hex is read by unhex, the reader the students use themselves."""
    path = os.path.join(opts["answers"], "attack%s.txt" % phase)
    try:
        with open(path, "rb") as f:
            text = f.read()
    except FileNotFoundError:
        return path, None
    except OSError as e:
        refuse("cannot read %s: %s" % (path, e))
    try:
        proc = subprocess.run([opts["unhex"]], input=text,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except OSError as e:
        refuse("cannot run %s: %s -- run make attack first" % (opts["unhex"], e))
    if proc.returncode != 0:
        refuse("%s would not read %s:\n%s" % (opts["unhex"], path,
               proc.stderr.decode("utf-8", "replace").rstrip()))
    return path, proc.stdout or None


def post(opts, endpoint, body):
    """The reply text of one endpoint, or a refusal."""
    host, sep, port = opts["server"].rpartition(":")
    if not host or not port.isdigit():
        fail("--server wants HOST:PORT, not %s" % opts["server"])
    try:
        timeout = float(opts["timeout"])
    except ValueError:
        fail("--timeout wants seconds, not %s" % opts["timeout"])
    if opts["scheme"] == "https":
        conn = http.client.HTTPSConnection(host, int(port), timeout=timeout,
                                           context=ssl.create_default_context())
    elif opts["scheme"] == "http":
        conn = http.client.HTTPConnection(host, int(port), timeout=timeout)
    else:
        fail("--scheme is https or http, not %s" % opts["scheme"])
    try:
        conn.request("POST", endpoint, body)
        reply = conn.getresponse()
        text = reply.read().decode("utf-8", "replace")
    except (OSError, http.client.HTTPException) as e:
        refuse("could not reach %s://%s: %s" % (opts["scheme"], opts["server"], e))
    finally:
        conn.close()
    if reply.status != 200:
        refuse("%s://%s said %s:\n%s" % (opts["scheme"], opts["server"],
                                         reply.status, text.rstrip()))
    return text


def main(argv):
    opts, sid, phases = parse_args(argv[1:])
    sent = passed = 0

    for phase in phases:
        path, data = read_answer(opts, phase)
        if data is None:
            print("%s: no answer to send" % path)
            continue
        text = post(opts, "/report",
                    ("id=%s\nphase=%s\n" % (sid, phase)).encode() + data).strip()
        print(text)
        sent += 1
        if re.match(r"phase %s: pass\b" % phase, text):
            passed += 1

    if sent == 0:
        refuse("no answer to submit")
    if sent > 1:
        print("%d of %d passed" % (passed, sent))
    return 0 if passed == sent else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
