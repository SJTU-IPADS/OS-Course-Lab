"""Auto-bold the flush-left prose lines of a `slide` block.

A lecture slide is written as a headline claim with supporting bullets beneath
it. `autobold` bolds the headline so the author does not repeat `**...**` on
every slide. Indenting a line opts it out.
"""

import re

_FENCE = re.compile(r"^(?:```|~~~)")
_SKIP = re.compile(
    r"""^(?:
        [-*+]\s          # bullet list item
      | \d+[.)]\s        # ordered list item
      | \#               # heading
      | >                # blockquote
      | \|               # table row
      | -{3,}\s*$        # thematic break
      | !                # image
      | <(?!mark\b)      # raw HTML — but `<mark>` is DSL markup, not the author
                         # taking over the line, and a headline whose *first*
                         # word is the point is the commonest place to mark one.
    )""",
    re.VERBOSE,
)


def _skip(line: str) -> bool:
    return (
        not line.strip()
        or line[:1].isspace()
        or "**" in line
        or "__" in line
        or bool(_SKIP.match(line))
    )


def autobold(content: str) -> str:
    out = []
    in_fence = False
    for line in content.split("\n"):
        if _FENCE.match(line):
            in_fence = not in_fence
            out.append(line)
        elif in_fence or _skip(line):
            out.append(line)
        else:
            trailing = line[len(line.rstrip()) :]
            out.append(f"**{line.strip()}**{trailing}")
    return "\n".join(out)
