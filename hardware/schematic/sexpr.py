"""S-expression parser and KiCad-faithful serializer.

The `.kicad_sch` format is a nested S-expression. The whole point of this module
is a *lossless* round-trip: parse a file and serialize it back byte-for-byte, so
that regenerating a schematic never produces spurious diffs (and never churns the
UUIDs that link the schematic to the PCB layout).

The trick that makes byte-exact round-trip practical is that every leaf atom keeps
its *original source text* (`raw`). We never reformat a number we merely read, so
`114.3` stays `114.3` and `0` stays `0`. Only atoms the DSL *creates* get formatted
(see `atom()` / `num()` / `qstr()`), and those follow KiCad's conventions.

KiCad's pretty-printer, reverse-engineered from the real board files:
  * Line ending is CRLF, indentation is tabs.
  * A list whose children (after the tag) are all atoms prints inline: `(at 1 2 0)`.
  * Otherwise it prints multi-line: the tag plus any *leading* atom children stay on
    the opening line, then each remaining child goes on its own indented line, and the
    closing paren sits alone at the parent indent.
  * `(pts ...)` is special: its `(xy ...)` children are packed up to 6 per line.
"""

from __future__ import annotations

NL = "\r\n"
INDENT = "\t"
PTS_PER_LINE = 6


class Atom:
    """A leaf token. `raw` is the exact source text (with quotes/escapes for
    strings); emitting `raw` verbatim is what guarantees byte-exact round-trip."""

    __slots__ = ("raw", "quoted")

    def __init__(self, raw: str, quoted: bool):
        self.raw = raw
        self.quoted = quoted

    @property
    def value(self) -> str:
        """Decoded value (quotes stripped, escapes resolved) for logic/lookup."""
        if not self.quoted:
            return self.raw
        s = self.raw[1:-1]
        return s.replace("\\\\", "\\").replace('\\"', '"').replace("\\n", "\n")

    def text(self) -> str:
        return self.raw

    def __repr__(self):
        return f"Atom({self.raw!r})"


class Node:
    """A list `(tag child child ...)`. `items[0]` is the tag atom."""

    __slots__ = ("items",)

    def __init__(self, items):
        self.items = items

    @property
    def tag(self) -> str:
        first = self.items[0]
        return first.value if isinstance(first, Atom) else ""

    # --- convenience accessors used by the DSL / migrator ---

    def find_all(self, tag):
        return [c for c in self.items[1:]
                if isinstance(c, Node) and c.tag == tag]

    def find(self, tag):
        for c in self.find_all(tag):
            return c
        return None

    def __repr__(self):
        return f"Node({self.tag!r}, {len(self.items) - 1} children)"


# --------------------------------------------------------------------------- #
# Parsing
# --------------------------------------------------------------------------- #

_DELIM = set(" \t\r\n()\"")


def _tokenize(s: str):
    i, n = 0, len(s)
    while i < n:
        c = s[i]
        if c in " \t\r\n":
            i += 1
            continue
        if c in "()":
            yield (c, c)
            i += 1
            continue
        if c == '"':
            j = i + 1
            while j < n:
                if s[j] == "\\":
                    j += 2
                    continue
                if s[j] == '"':
                    j += 1
                    break
                j += 1
            yield ("str", s[i:j])
            i = j
            continue
        j = i
        while j < n and s[j] not in _DELIM:
            j += 1
        yield ("sym", s[i:j])
        i = j


def loads(text: str) -> Node:
    """Parse one S-expression (the whole file) into a Node tree."""
    tokens = list(_tokenize(text))
    pos = 0

    def read():
        nonlocal pos
        kind, val = tokens[pos]
        pos += 1
        if kind == "(":
            items = []
            while tokens[pos][0] != ")":
                items.append(read())
            pos += 1  # consume ')'
            return Node(items)
        if kind == ")":
            raise ValueError("unexpected ')'")
        return Atom(val, quoted=(kind == "str"))

    root = read()
    return root


def load(path) -> Node:
    with open(path, "r", encoding="utf-8", newline="") as f:
        return loads(f.read())


# --------------------------------------------------------------------------- #
# Serializing (KiCad-faithful)
# --------------------------------------------------------------------------- #

def _fmt(node, depth: int) -> str:
    if isinstance(node, Atom):
        return node.text()

    items = node.items
    tag, rest = items[0], items[1:]

    # Inline when nothing nested.
    if all(isinstance(e, Atom) for e in rest):
        return "(" + " ".join(_fmt(e, 0) for e in items) + ")"

    child_indent = INDENT * (depth + 1)
    close_indent = INDENT * depth

    # pts packs its xy children 6 to a line.
    if isinstance(tag, Atom) and tag.value == "pts":
        out = ["(" + _fmt(tag, 0)]
        line = []
        for e in rest:
            line.append(_fmt(e, 0))
            if len(line) == PTS_PER_LINE:
                out.append(child_indent + " ".join(line))
                line = []
        if line:
            out.append(child_indent + " ".join(line))
        return NL.join(out) + NL + close_indent + ")"

    # General multi-line: leading atoms ride on the tag line.
    lead = []
    i = 0
    while i < len(rest) and isinstance(rest[i], Atom):
        lead.append(rest[i])
        i += 1
    first = "(" + _fmt(tag, 0)
    if lead:
        first += " " + " ".join(_fmt(a, 0) for a in lead)
    out = [first]
    for e in rest[i:]:
        out.append(child_indent + _fmt(e, depth + 1))
    out.append(close_indent + ")")
    return NL.join(out)


def dumps(root: Node) -> str:
    """Serialize a Node tree to KiCad's exact text form (trailing CRLF included)."""
    return _fmt(root, 0) + NL


def dump(root: Node, path) -> None:
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(dumps(root))


# --------------------------------------------------------------------------- #
# Atom constructors (for content the DSL creates, not content we round-trip)
# --------------------------------------------------------------------------- #

def _fmt_number(x) -> str:
    """KiCad-style number: integers without a decimal point, no trailing zeros."""
    if isinstance(x, int):
        return str(x)
    s = f"{x:.6f}".rstrip("0").rstrip(".")
    return s if s not in ("", "-0") else "0"


def num(x) -> Atom:
    return Atom(_fmt_number(x), quoted=False)


def sym(name: str) -> Atom:
    return Atom(name, quoted=False)


def qstr(value: str) -> Atom:
    esc = value.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
    return Atom(f'"{esc}"', quoted=True)


def node(tag: str, *items) -> Node:
    return Node([sym(tag), *items])
