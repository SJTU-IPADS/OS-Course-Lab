"""A deliberately small BibTeX reader for ``p.cite("@…{…}")``.

Not a full BibTeX parser: it lifts the handful of fields a citation line needs
(title / author / year / venue / url), applies a *common-case* beautification
pass (author → "Last et al.", a small slice of LaTeX escapes, protective-brace
stripping), and returns a plain dict. Anything it cannot make sense of is left
out rather than raised on, so the author's explicit keyword fields can fill the
gap. See ``dsl.PageBuilder.cite``.
"""

from __future__ import annotations

import re

# Map bibtex field names onto the citation fields we keep. The first entry wins
# when several bibtex fields target the same slot (journal before booktitle).
_VENUE_FIELDS = ("journal", "booktitle", "venue")
_KNOWN = ("title", "author", "year", "url")

_ENTRY_RE = re.compile(r"@\s*\w+\s*\{\s*([^,\s}]*)\s*,(.*)\}\s*$", re.DOTALL)

# A small, common LaTeX-escape subset. Accents are handled separately.
_ESCAPES = (
    (r"\&", "&"),
    (r"\%", "%"),
    (r"\_", "_"),
    (r"\#", "#"),
    (r"\$", "$"),
    ("---", "—"),
    ("--", "–"),
    ("~", " "),
)

# {\"o} / \"o / {\'e} … → accented letter. Covers the handful that show up in
# author names; unknown accents fall through and get their braces stripped.
_ACCENTS = {
    '"': {"a": "ä", "o": "ö", "u": "ü", "e": "ë", "i": "ï", "A": "Ä", "O": "Ö", "U": "Ü"},
    "'": {"a": "á", "e": "é", "i": "í", "o": "ó", "u": "ú", "n": "ń", "c": "ć"},
    "`": {"a": "à", "e": "è", "i": "ì", "o": "ò", "u": "ù"},
    "^": {"a": "â", "e": "ê", "i": "î", "o": "ô", "u": "û"},
    "~": {"n": "ñ", "a": "ã", "o": "õ"},
}
_ACCENT_RE = re.compile(r'\{?\\(["\'`^~])\s*\{?(\w)\}?\}?')


def parse(text: str) -> dict:
    """Lift citation fields from a BibTeX entry string. ``{}`` if it isn't one."""
    match = _ENTRY_RE.search(text.strip())
    if not match:
        return {}
    key, body = match.group(1), match.group(2)
    raw = _split_fields(body)

    fields: dict[str, str] = {}
    if key:
        fields["key"] = key
    for name in _KNOWN:
        if name in raw:
            fields[name] = _clean(raw[name])
    for name in _VENUE_FIELDS:
        if name in raw:
            fields["venue"] = _clean(raw[name])
            break
    if "author" in fields:
        fields["author"] = _beautify_authors(fields["author"])
    return fields


def _split_fields(body: str) -> dict[str, str]:
    """``field = value`` pairs, split on brace-balanced top-level commas."""
    raw: dict[str, str] = {}
    for chunk in _top_level_commas(body):
        if "=" not in chunk:
            continue
        name, _, value = chunk.partition("=")
        name = name.strip().lower()
        if name:
            raw[name] = _unwrap(value.strip())
    return raw


def _top_level_commas(body: str) -> list[str]:
    """Split ``body`` on commas that sit outside any ``{…}`` / ``"…"``."""
    parts: list[str] = []
    depth = 0
    in_quote = False
    start = 0
    for i, ch in enumerate(body):
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth = max(0, depth - 1)
        elif ch == '"' and depth == 0:
            in_quote = not in_quote
        elif ch == "," and depth == 0 and not in_quote:
            parts.append(body[start:i])
            start = i + 1
    parts.append(body[start:])
    return parts


def _unwrap(value: str) -> str:
    """Strip one wrapping ``{…}`` or ``"…"`` from a field value."""
    value = value.strip()
    if len(value) >= 2 and value[0] == "{" and value[-1] == "}":
        return value[1:-1]
    if len(value) >= 2 and value[0] == '"' and value[-1] == '"':
        return value[1:-1]
    return value


def _clean(value: str) -> str:
    """Turn a raw field value into display text (escapes, braces, whitespace)."""
    value = _ACCENT_RE.sub(lambda m: _ACCENTS.get(m.group(1), {}).get(m.group(2), m.group(2)), value)
    for src, dst in _ESCAPES:
        value = value.replace(src, dst)
    value = value.replace("{", "").replace("}", "")
    return re.sub(r"\s+", " ", value).strip()


def _beautify_authors(author: str) -> str:
    """``A and B and others`` → a short form: single name, or "Last et al."."""
    names = [n.strip() for n in re.split(r"\s+and\s+", author) if n.strip()]
    trailing_others = bool(names) and names[-1].lower() in ("others", "et al.", "et al")
    real = [n for n in names if n.lower() not in ("others", "et al.", "et al")]
    if not real:
        return author
    if len(real) == 1 and not trailing_others:
        return _display_name(real[0])
    return f"{_last_name(real[0])} et al."


def _display_name(name: str) -> str:
    """"Last, First" → "First Last"; a plain "First Last" is left as-is."""
    if "," in name:
        last, _, first = name.partition(",")
        return f"{first.strip()} {last.strip()}".strip()
    return name


def _last_name(name: str) -> str:
    if "," in name:
        return name.split(",", 1)[0].strip()
    return name.split()[-1] if name.split() else name
