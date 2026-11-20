"""Where a `$…$` formula is in a line of the DSL's markdown.

The deck's markdown engine decides this in the viewer (Marp's math rule, the
one pandoc uses), and every other target must read the same text the same way,
or a sentence about prices, "$55 和 $4", is prose in the deck and a formula in
the PowerPoint and the book. The rule:

- the opening `$` is followed by a character that is not a space;
- the closing `$` is the next `$`, follows a character that is not a space,
  and is not followed by a digit.

A `$` that opens nothing is a literal dollar sign: "$55 和 $4" and "$8，$10"
are text, "$a $b$" is the text "$a " and the formula `b`.

``INLINE`` is the pattern of one such span, its LaTeX in the group ``math``;
the renderers put it into their own inline grammars after their `$$…$$`
alternative.
"""

INLINE = r"\$(?![\s$])(?P<math>[^$\n]*?[^\s$])\$(?!\d)"
