#!/usr/bin/env python3
"""Adapt the supplied Lab3/Lab5 shell binary to Lab6's raspi4 console.

Only handle_uart_irq() is replaced. The original input buffering, foreground
process handling, and command implementation are reused. The replacement calls
handle_uart_irq_internal(), then usleep(1000), instead of waiting for IRQ 57.
Lab6's raspi4 kernel uses the mini UART with receive interrupts disabled.

The shell source is not included in the labs, so the input hash and offsets are
pinned to the provided ELF. A different shell must be inspected separately.
"""

import argparse
import hashlib
from pathlib import Path
import struct


SOURCE_SHA256 = "45c771982e03f87d76e1a5a588ba4d423d09f72cea088eed71f186b3b9a4f89d"
FUNCTION_OFFSET = 0x3D40
FUNCTION_SIZE = 0x5C

# AArch64 instructions linked at the original handle_uart_irq() address.
# The initial executable PT_LOAD has p_offset == p_vaddr == 0, so the virtual
# addresses below are also file offsets in this specific ELF.
POLL_FUNCTION = struct.pack(
    "<6I",
    0xA9BF7BFD,  # 0x3d40: stp x29, x30, [sp, #-16]!
    0x910003FD,  # 0x3d44: mov x29, sp
    0x97FFFFB4,  # 0x3d48: bl 0x3c18 (handle_uart_irq_internal)
    0x52807D00,  # 0x3d4c: mov w0, #1000
    0x97FFF680,  # 0x3d50: bl 0x1750 (usleep@plt)
    0x17FFFFFD,  # 0x3d54: b 0x3d48
)
NOP = struct.pack("<I", 0xD503201F)


def patch_shell(data):
    digest = hashlib.sha256(data).hexdigest()
    if digest != SOURCE_SHA256:
        raise ValueError(
            f"Unsupported source shell SHA-256: {digest}; expected {SOURCE_SHA256}"
        )

    replacement = POLL_FUNCTION + NOP * ((FUNCTION_SIZE - len(POLL_FUNCTION)) // 4)
    return (
        data[:FUNCTION_OFFSET]
        + replacement
        + data[FUNCTION_OFFSET + FUNCTION_SIZE :]
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="Original Lab3/Lab5 chcore_shell.bin")
    parser.add_argument("output", type=Path, help="Destination raspi4 chcore_shell.bin")
    args = parser.parse_args()

    if args.source.resolve() == args.output.resolve():
        parser.error("Source and output must be different files")

    try:
        result = patch_shell(args.source.read_bytes())
        args.output.write_bytes(result)
        args.output.chmod(0o755)
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")

    print(f"Wrote {args.output} ({len(result)} bytes)")
    print(f"SHA-256: {hashlib.sha256(result).hexdigest()}")


if __name__ == "__main__":
    main()
