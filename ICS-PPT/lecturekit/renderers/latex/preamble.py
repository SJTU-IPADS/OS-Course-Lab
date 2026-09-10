"""The book's LaTeX preamble and its Makefile.

`ctexbook` + XeLaTeX: the body is Chinese, so `book` plus a CJK package is
unavoidable and `ctex` is the standard choice. Fonts are left to ctex's
system defaults, mirroring how the viewer uses system fonts.
"""

from __future__ import annotations

from ... import i18n, tokens
from .text import escape

_PACKAGES = """\
\\usepackage{graphicx}
\\usepackage{booktabs}
\\usepackage{listings}
\\usepackage{xcolor}
\\usepackage[most]{tcolorbox}
\\usepackage{subcaption}
\\usepackage{hyperref}
"""

# `@--token@` below names one of the theme's design tokens, which
# `tokens.substitute` fills in — a colour composited onto white, a length as
# written. The book prints no colour or stroke geometry of its own: a marked
# keyword, a chip, a state name in pseudocode are the deck's inks, so a reader
# who studied the slides meets the same page in print.
# `@--token@` is a theme token filled in by `tokens.substitute`; `{todo}` is the
# book's own TODO heading, which follows the book's language (see `i18n`).
_MACROS = tokens.substitute("""\
% Code listings: wrap long lines, keep UTF-8 intact.
\\lstset{
  basicstyle=\\ttfamily\\small,
  breaklines=true,
  frame=single,
  showstringspaces=false,
  extendedchars=true,
}

% Pseudocode. `listings` is not used here: the `pseudo` language has no grammar
% to give it, and what wants colour is the protocol's vocabulary rather than
% keyword/string/comment. lecturekit lexes it and emits the tokens wrapped in
% these macros; the colours match the deck's, so a message or a state name
% looks the same in the book as on the slide.
\\definecolor{lkPseudoKw}{HTML}{@--pseudo-keyword@}
\\definecolor{lkPseudoMsg}{HTML}{@--pseudo-message@}
\\definecolor{lkPseudoState}{HTML}{@--pseudo-state@}
\\definecolor{lkPseudoNote}{HTML}{@--code-comment@}
\\newcommand{\\lkpskw}[1]{\\textcolor{lkPseudoKw}{\\bfseries #1}}
\\newcommand{\\lkpsmsg}[1]{\\textcolor{lkPseudoMsg}{#1}}
\\newcommand{\\lkpsst}[1]{\\textcolor{lkPseudoState}{\\bfseries #1}}
\\newcommand{\\lkpscm}[1]{\\textcolor{lkPseudoNote}{\\itshape #1}}
\\newenvironment{lkpseudo}{%
  \\par\\medskip\\begin{flushleft}\\ttfamily\\small}{%
  \\end{flushleft}\\medskip}

% A marked pseudocode line (`p.code(..., mark=[…])`): the marker pen at line
% scale, in the same \\lkMark* inks the inline `<mark>` below is drawn with — a
% marked word and a marked line are one gesture at two scales. \\fboxsep is
% zeroed so the wash adds no width: a marked line's indentation stays flush
% with its neighbours.
\\newcommand{\\lkpshl}[2]{{\\setlength{\\fboxsep}{0pt}\\colorbox{#1}{\\strut #2}}}

% An unwritten page keeps its heading and its figures, and says so.
\\newcommand{\\booktodo}[1]{%
  \\begin{tcolorbox}[colback=red!5,colframe=red!60,title=@@TODO@@]%
  This page has no \\texttt{prose} block yet (page id: \\texttt{#1}).%
  \\end{tcolorbox}}

% A figure whose source format print cannot carry (an animated GIF, say).
\\newcommand{\\bookunrenderable}[1]{%
  \\fbox{\\parbox{0.8\\textwidth}{\\centering\\small
  [figure not renderable in print: \\texttt{#1}]}}}

% A sidenote: boxed callout pointing at external material.
\\newenvironment{booksidenote}[1]{%
  \\begin{tcolorbox}[colback=black!3,colframe=black!40,title={#1}]}{%
  \\end{tcolorbox}}

% A highlight: the deck's punchline line — a hairline frame in the tone, the
% text inside set in that same tone, nothing painted behind it, and the soft
% shadow the deck's chip carries. The one-column tabular auto-sizes to the
% widest line, so a multi-line chip grows taller rather than wider, and
% \\tcbox draws the frame around exactly that instead of spanning the text
% width. (`most` is loaded above, which brings the skins
% library the shadow needs.)
% \\large\\bfseries is set *before* the tabular: a font command inside the first
% cell is scoped to that cell, so later rows would fall back to body size, and
% \\baselineskip (the row spacing) must already be \\large's when the tabular
% starts.
% The optional first argument carries footnote marks, so they sit just outside
% the chip on its own line — as in the deck — instead of after \\end{center},
% where they would start a paragraph and land in the margin.
\\definecolor{lkChipYellow}{HTML}{@--chip-yellow@}
\\definecolor{lkChipOrange}{HTML}{@--chip-orange@}
\\definecolor{lkChipGreen}{HTML}{@--chip-green@}
\\definecolor{lkChipBlue}{HTML}{@--chip-blue@}
\\newcommand{\\lkhighlight}[3][]{%
  \\begin{center}%
  \\tcbox[enhanced, colback=white, colframe=#2, boxrule=0.6pt,
    left=6pt, right=6pt, top=4pt, bottom=4pt, sharp corners,
    drop fuzzy shadow=black!20]{%
    \\large\\bfseries\\color{#2}\\begin{tabular}{@{}c@{}}#3\\end{tabular}}#1%
  \\end{center}}

% An inline keyword highlight (`<mark>`): a marker stroke over the body of the
% word, as in the deck. The word is boxed first only to measure it, then
% a rule of exactly that width is \\rlap'd behind it — \\rlap contributes no
% width, and the text is typeset after the rule, so it prints on top. The rule
% runs from just under the baseline to the top of the x-height (the theme's
% --mark-tex-raise / --mark-tex-height, its ex-unit statement of the deck's
% --mark-top / --mark-bottom gradient): struck through the middle of the
% lowercase instead, the stroke is half a stroke and a run of capitals gets
% only its bottom third.
% PDF colour is opaque where this rule is drawn, so the deck's translucent
% strokes arrive composited onto white; the text over them is unaffected.
% \\strut fixes the stroke to the line's own height, so a sentence holding
% several marks does not ripple with the ascenders and descenders each one
% happens to contain. The box is unbreakable, which is correct here — a marked
% span is a keyword, and a keyword should not be hyphenated across a line.
\\definecolor{lkMarkYellow}{HTML}{@--stroke-yellow@}
\\definecolor{lkMarkOrange}{HTML}{@--stroke-orange@}
\\definecolor{lkMarkGreen}{HTML}{@--stroke-green@}
\\definecolor{lkMarkBlue}{HTML}{@--stroke-blue@}
\\newsavebox{\\lkmarkbox}
\\newcommand{\\lkmark}[2]{{%
  \\sbox{\\lkmarkbox}{\\strut #2}%
  \\rlap{\\textcolor{#1}{\\rule[@--mark-tex-raise@]{\\wd\\lkmarkbox}{@--mark-tex-height@}}}\\usebox{\\lkmarkbox}}}
""")

MAKEFILE = """\
book.pdf: book.tex
\tlatexmk -xelatex -interaction=nonstopmode book.tex

clean:
\tlatexmk -C
.PHONY: clean
"""


def document_preamble(book) -> str:
    """Everything from ``\\documentclass`` up to (not including) ``\\begin{document}``."""
    lines = [
        "\\documentclass[11pt]{ctexbook}",
        "",
        _PACKAGES,
        _MACROS.replace("@@TODO@@", i18n.ui(book.lang, "todo")),
        f"\\title{{{escape(book.title)}}}",
    ]
    if book.subtitle:
        lines.append(f"\\date{{{escape(book.subtitle)}}}")
    if book.author:
        lines.append(f"\\author{{{escape(book.author)}}}")
    return "\n".join(lines)
