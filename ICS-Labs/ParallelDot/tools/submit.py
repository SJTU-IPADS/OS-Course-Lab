#!/usr/bin/env python3
"""submit.py -- send your kernel file of step 2 to the course server
and print what it sends back. make remote runs it.

usage: python3 tools/submit.py [--server URL] [--token TOKEN] [PATH]

PATH is the file ops/cuda.cu (the default) or a directory holding a
file cuda.cu. The file is packed into a tar archive and posted to
URL/run; the server's reply is printed line by line as it arrives. On
the server the file takes the place of ops/cuda.cu in a fresh copy of
this directory, and sh run.sh runs there: it builds the two programs of
step 2, runs ops-selftest-cuda, then mini-ollama-cuda on the server's
copy of the model file. The reply holds what a local sh run.sh prints.

--server defaults to the environment variable GPU_SERVER, then to
http://localhost:8080; --token to GPU_TOKEN.

Exit status: 0 when the server reports status=ok; 1 when the job
failed on the server (a program exited with an error), 2 when the job
was killed (time or output limit), 3 when the server refused the
submission or could not be reached.

Only the Python standard library is used."""
import io
import os
import sys
import tarfile
import urllib.error
import urllib.request

ALLOWED = ("cuda.cu",)


def collect(paths):
    """The file named by paths: a file is taken as it is, a directory
    contributes the allowed name it holds."""
    files = {}
    for p in paths:
        if os.path.isdir(p):
            for name in ALLOWED:
                f = os.path.join(p, name)
                if os.path.isfile(f):
                    files[name] = f
        elif os.path.isfile(p):
            name = os.path.basename(p)
            if name not in ALLOWED:
                sys.exit("submit.py: %s: only %s can be submitted" % (p, ", ".join(ALLOWED)))
            files[name] = p
        else:
            sys.exit("submit.py: %s: no such file or directory" % p)
    if not files:
        sys.exit("submit.py: no file %s found" % ", ".join(ALLOWED))
    return files


def pack(files):
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode="w") as tar:
        for name, path in sorted(files.items()):
            tar.add(path, arcname=name, recursive=False)
    return buf.getvalue()


def main(argv):
    server = os.environ.get("GPU_SERVER", "http://localhost:8080")
    token = os.environ.get("GPU_TOKEN", "")
    paths = []
    args = argv[1:]
    while args:
        a = args.pop(0)
        if a == "--server" and args:
            server = args.pop(0)
        elif a == "--token" and args:
            token = args.pop(0)
        elif a.startswith("-"):
            sys.stderr.write(__doc__ + "\n")
            return 3
        else:
            paths.append(a)
    body = pack(collect(paths or [os.path.join("ops", "cuda.cu")]))

    req = urllib.request.Request(server.rstrip("/") + "/run", data=body, method="POST")
    req.add_header("Content-Type", "application/x-tar")
    if token:
        req.add_header("X-Token", token)
    status = 3
    try:
        with urllib.request.urlopen(req) as resp:
            for raw in resp:
                line = raw.decode("utf-8", "replace")
                sys.stdout.write(line)
                sys.stdout.flush()
                if line.startswith("# done "):
                    for item in line.split():
                        if item.startswith("status="):
                            status = {"ok": 0, "error": 1, "timeout": 2,
                                      "output-limit": 2}.get(item[7:], 1)
    except urllib.error.HTTPError as e:
        sys.stderr.write("submit.py: server refused the submission: %d %s\n%s"
                         % (e.code, e.reason, e.read().decode("utf-8", "replace")))
        return 3
    except urllib.error.URLError as e:
        sys.stderr.write("submit.py: cannot reach %s: %s\n" % (server, e.reason))
        return 3
    return status


if __name__ == "__main__":
    sys.exit(main(sys.argv))
