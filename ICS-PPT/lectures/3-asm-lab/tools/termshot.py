#!/usr/bin/env python3
"""termshot.py -- run a program on a pseudo-terminal of a fixed size, drive it
with a few keystrokes, and draw what the terminal shows as a PNG.

    python3 termshot.py --out shot.png [--rows 24] [--cols 80] [--font-size 20]
        [--wait TEXT] [--type TEXT] [--key NAME] [--pause SECONDS] ...
        [--keep-raw FILE] -- PROGRAM [ARG...]

The steps --wait, --type, --key and --pause run in the order they are given:
--wait collects output until TEXT has appeared (5 s at most), --type sends
TEXT as keystrokes once the program reads the keyboard key by key, --key
sends one named key (enter, ctrl-c, ctrl-d, q, ...), --pause waits SECONDS
while collecting output. After the last step the program is sent SIGHUP (the
terminal goes away), the bytes it wrote are replayed through the terminal
emulator pyte, and the screen is drawn cell by cell with the DejaVu Sans Mono
fonts of the system.

A program that leaves the alternate screen (ESC [?1049 l) restores the shell's
screen; the bytes from that point on are dropped, so the picture is the last
full screen the program drew. The program gets 512 MB of memory at most and is
killed when it does not end after the terminal has gone.

Needs pyte (pip install pyte) and Pillow.
"""
import argparse
import errno
import fcntl
import os
import pty
import resource
import select
import signal
import struct
import sys
import termios
import time

import pyte
from PIL import Image, ImageDraw, ImageFont

MEMORY = 512 << 20
FONT_DIR = "/usr/share/fonts/truetype/dejavu"
FONT = os.path.join(FONT_DIR, "DejaVuSansMono.ttf")
FONT_BOLD = os.path.join(FONT_DIR, "DejaVuSansMono-Bold.ttf")

# A dark terminal: the background, the default text, and the 16 colours.
BACKGROUND = "#1b1d23"
FOREGROUND = "#d6d8dd"
ANSI = {
    "black": "#2b2e36", "red": "#e5484d", "green": "#4fb24f", "brown": "#d9a53b",
    "blue": "#5b8def", "magenta": "#c678dd", "cyan": "#3fb8c4", "white": "#c9ccd3",
    "brightblack": "#6b7280", "brightred": "#ff6b70", "brightgreen": "#7ad17a",
    "brightyellow": "#f0c050", "brightblue": "#82a8ff", "brightmagenta": "#dd9cf0",
    "brightcyan": "#6fd3dd", "brightwhite": "#f2f3f5",
}
KEYS = {
    "enter": b"\r", "tab": b"\t", "escape": b"\x1b", "backspace": b"\x7f",
    "ctrl-c": b"\x03", "ctrl-d": b"\x04", "ctrl-l": b"\x0c",
    "up": b"\x1b[A", "down": b"\x1b[B", "right": b"\x1b[C", "left": b"\x1b[D",
    "pgup": b"\x1b[5~", "pgdn": b"\x1b[6~", "home": b"\x1b[H", "end": b"\x1b[F",
}
LEAVE = b"\x1b[?1049l"


class Step(argparse.Action):
    """Record the steps in the order they appear on the command line."""

    def __call__(self, parser, namespace, value, option):
        steps = getattr(namespace, "steps", None) or []
        steps.append((option.lstrip("-"), value))
        namespace.steps = steps


def parse(argv):
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--out", required=True, help="the PNG to write")
    ap.add_argument("--rows", type=int, default=24)
    ap.add_argument("--cols", type=int, default=80)
    ap.add_argument("--font-size", type=int, default=20)
    ap.add_argument("--keep-raw", help="also save the bytes the program wrote")
    ap.add_argument("--env", action="append", default=[], metavar="NAME=VALUE",
                    help="an environment variable for the program")
    ap.add_argument("--cwd", help="the directory the program runs in")
    ap.add_argument("--wait", action=Step, metavar="TEXT")
    ap.add_argument("--type", action=Step, metavar="TEXT")
    ap.add_argument("--key", action=Step, metavar="NAME", choices=sorted(KEYS))
    ap.add_argument("--pause", action=Step, metavar="SECONDS")
    ap.add_argument("program", nargs="+")
    args = ap.parse_args(argv)
    if not getattr(args, "steps", None):
        args.steps = []
    return args


# ------------------------------------------------------------ the terminal

def start(program, rows, cols, extra_env, cwd):
    pid, master = pty.fork()
    if pid == 0:
        resource.setrlimit(resource.RLIMIT_AS, (MEMORY, MEMORY))
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
        fcntl.ioctl(0, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))
        env = {"TERM": "xterm-256color", "LANG": "C.UTF-8", "COLUMNS": str(cols),
               "LINES": str(rows), "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
               "HOME": os.environ.get("HOME", "/")}
        for item in extra_env:
            name, _, value = item.partition("=")
            env[name] = value
        if cwd:
            os.chdir(cwd)
        try:
            os.execvpe(program[0], program, env)
        except OSError as e:
            sys.stderr.write("termshot: %s: %s\n" % (program[0], e))
            os._exit(127)
    return pid, master


def collect(master, seen, text=None, seconds=5.0):
    """Read what the program writes until text (bytes) has appeared, the
    program has closed the terminal, or the time is over."""
    end = time.monotonic() + seconds
    while (text is None or text not in seen) and time.monotonic() < end:
        ready, _, _ = select.select([master], [], [], 0.05)
        if not ready:
            continue
        try:
            chunk = os.read(master, 1 << 16)
        except OSError as e:
            if e.errno != errno.EIO:
                raise
            break
        if not chunk:
            break
        seen += chunk
    return seen


def raw_mode(master, seconds=5.0):
    """True once the program reads the keyboard key by key."""
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        try:
            if not termios.tcgetattr(master)[3] & termios.ICANON:
                return True
        except termios.error:
            return False
        time.sleep(0.01)
    return False


def finish(pid, master, seconds=3.0):
    """Hang the terminal up and wait for the program; kill it when it stays."""
    try:
        os.close(master)
    except OSError:
        pass
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        done, _ = os.waitpid(pid, os.WNOHANG)
        if done:
            return
        time.sleep(0.02)
    os.kill(pid, signal.SIGKILL)
    os.waitpid(pid, 0)


def drive(args):
    pid, master = start(args.program, args.rows, args.cols, args.env, args.cwd)
    seen = b""
    try:
        for kind, value in args.steps:
            if kind == "wait":
                seen = collect(master, seen, value.encode())
                if value.encode() not in seen:
                    sys.stderr.write("termshot: %r did not appear\n" % value)
            elif kind == "type":
                raw_mode(master)
                os.write(master, value.encode())
                seen = collect(master, seen, None, 0.3)
            elif kind == "key":
                raw_mode(master)
                os.write(master, KEYS[value])
                seen = collect(master, seen, None, 0.3)
            elif kind == "pause":
                seen = collect(master, seen, None, float(value))
    finally:
        finish(pid, master)
    return seen


# -------------------------------------------------------------- the picture

def colour(name, default):
    if name == "default":
        return default
    if name in ANSI:
        return ANSI[name]
    if len(name) == 6:
        return "#" + name
    return default


# Box-drawing characters are drawn as lines from the middle of the cell to
# its edges (left, right, up, down), so that they join without gaps.
BOX = {
    "─": "lr", "│": "ud", "┌": "rd", "┐": "ld", "└": "ru", "┘": "lu",
    "├": "rud", "┤": "lud", "┬": "lrd", "┴": "lru", "┼": "lrud",
    "╭": "rd", "╮": "ld", "╰": "ru", "╯": "lu",
}


def draw(screen, out, font_size):
    font = ImageFont.truetype(FONT, font_size)
    bold = ImageFont.truetype(FONT_BOLD, font_size)
    cw = int(round(font.getlength("M")))
    ch = int(round(font_size * 1.25))
    ascent, descent = font.getmetrics()
    pad = int(round(font_size * 0.6))
    width = screen.columns * cw + 2 * pad
    height = screen.lines * ch + 2 * pad
    im = Image.new("RGB", (width, height), BACKGROUND)
    d = ImageDraw.Draw(im)
    top_pad = (ch - (ascent + descent)) // 2
    stroke = max(1, font_size // 10)
    for row in range(screen.lines):
        line = screen.buffer[row]
        for col in range(screen.columns):
            c = line[col]
            fg = colour(c.fg, FOREGROUND)
            bg = colour(c.bg, BACKGROUND)
            if c.reverse:
                fg, bg = bg, fg
            x = pad + col * cw
            y = pad + row * ch
            if bg != BACKGROUND:
                d.rectangle([x, y, x + cw - 1, y + ch - 1], fill=bg)
            if not c.data or c.data == " ":
                continue
            if c.data in BOX:
                # Rectangles, not lines: a thick line drawn leftward lands
                # one pixel off the same line drawn rightward.
                mx, my = x + cw // 2, y + ch // 2
                h = stroke // 2
                for side in BOX[c.data]:
                    d.rectangle({"l": [x, my - h, mx, my + h],
                                 "r": [mx, my - h, x + cw, my + h],
                                 "u": [mx - h, y, mx + h, my],
                                 "d": [mx - h, my, mx + h, y + ch]}[side], fill=fg)
                continue
            d.text((x, y + top_pad), c.data, font=bold if c.bold else font, fill=fg)
    im.save(out)


def main(argv):
    args = parse(argv)
    seen = drive(args)
    if args.keep_raw:
        with open(args.keep_raw, "wb") as f:
            f.write(seen)
    # The program may enter and leave the alternate screen several times
    # (the bomb leaves it to echo the line it read, and enters it again for
    # the explosion). The picture is the screen before the last leave; the
    # clear the program sends just before leaving is dropped with it.
    left = seen.rfind(LEAVE)
    entered = seen.rfind(b"\x1b[?1049h")
    if 0 <= left and 0 <= entered < left:
        clear = seen.rfind(b"\x1b[2J", max(0, left - 40), left)
        seen = seen[:clear if clear >= 0 else left]
    screen = pyte.Screen(args.cols, args.rows)
    stream = pyte.ByteStream(screen)
    stream.feed(seen)
    draw(screen, args.out, args.font_size)
    print(args.out)


if __name__ == "__main__":
    main(sys.argv[1:])
