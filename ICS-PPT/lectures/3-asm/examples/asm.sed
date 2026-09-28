# Keep what a slide shows of gcc -S output: the instructions and the jump
# targets. Drops the assembler directives (.file, .cfi_*, .size, ...) and the
# .LFB/.LFE labels gcc puts around each function.
#   gcc -Og -fcf-protection=none -S dot.c -o - | sed -f asm.sed
/^[[:space:]]*\.[a-z]/d
/^\.LF[BE][0-9]*:/d
