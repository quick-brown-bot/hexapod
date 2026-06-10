"""Schematic DSL — author and edit KiCad schematics from Python.

A `Schematic` wraps the S-expression tree of a `.kicad_sch` file. You can start a
fresh board with `Schematic.new(...)` or load an existing one (the normal path for
a migrated board) with `Schematic.load(...)`. Either way, editing operations append
or modify nodes, every UUID is drawn from the `UuidRegistry`, and `.write()`
serializes back with the byte-faithful formatter in `sexpr`.

Why "load then edit" is the safe default for an existing board: the engine round-
trips untouched files byte-for-byte, so loading a migrated schematic and adding one
component leaves every other symbol — and therefore the entire PCB layout — exactly
as it was. Only genuinely new elements get new UUIDs.

Pin geometry: KiCad places a symbol by mapping each library pin coordinate through
`R(rotation) · mirror_Y`, then translating to the symbol's position. The four cases
in `_ROT` were validated against the real boards (pins landing exactly on existing
wire endpoints). This is what lets `net(...)` / `wire(...)` connect to a pin by name
without hand-computing coordinates.
"""

from __future__ import annotations

from pathlib import Path

from . import sexpr
from .sexpr import Atom, Node, node, num, qstr, sym
from .registry import UuidRegistry

# Library pin coord -> placement offset, by symbol rotation (degrees).
# Equivalent to standard-rotation R(theta) composed with KiCad's library Y-mirror.
_ROT = {
    0: lambda x, y: (x, -y),
    90: lambda x, y: (y, x),
    180: lambda x, y: (-x, y),
    270: lambda x, y: (-y, -x),
}


def _round(v: float) -> float:
    return round(v, 4)


class PlacedSymbol:
    """A component instance on the sheet. `pin(...)` returns world coordinates."""

    def __init__(self, schem: "Schematic", snode: Node, ref: str):
        self.schem = schem
        self.node = snode
        self.ref = ref

    @property
    def at(self):
        a = self.node.find("at")
        return float(a.items[1].value), float(a.items[2].value), int(float(a.items[3].value))

    def pin(self, number_or_name) -> tuple[float, float]:
        lib_id = self.node.find("lib_id").items[1].value
        local = self.schem._lib_pin(lib_id, str(number_or_name))
        if local is None:
            raise KeyError(f"{self.ref}: no pin {number_or_name!r} on {lib_id}")
        px, py, rot = self.at
        dx, dy = _ROT[rot % 360](local[0], local[1])
        return (_round(px + dx), _round(py + dy))


class Schematic:
    def __init__(self, root: Node, registry: UuidRegistry, name: str):
        self.root = root
        self.reg = registry
        self.name = name
        self._lib_pins_cache: dict[str, dict[str, tuple[float, float]]] = {}

    # ------------------------------------------------------------------ #
    # Construction
    # ------------------------------------------------------------------ #

    @classmethod
    def load(cls, path, registry: UuidRegistry, name: str | None = None) -> "Schematic":
        root = sexpr.load(path)
        name = name or Path(path).stem
        return cls(root, registry, name)

    @classmethod
    def new(cls, name: str, registry: UuidRegistry, paper: str = "A4") -> "Schematic":
        root_uuid = registry.get(f"root:{name}")
        root = node(
            "kicad_sch",
            node("version", sym("20250114")),
            node("generator", qstr("eeschema")),
            node("generator_version", qstr("9.0")),
            node("uuid", qstr(root_uuid)),
            node("paper", qstr(paper)),
            node("lib_symbols"),
            node("sheet_instances", node("path", qstr("/"), node("page", qstr("1")))),
            node("embedded_fonts", sym("no")),
        )
        return cls(root, registry, name)

    @property
    def root_uuid(self) -> str:
        return self.root.find("uuid").items[1].value

    def write(self, path) -> None:
        sexpr.dump(self.root, path)
        self.reg.save()

    # ------------------------------------------------------------------ #
    # Library / pin geometry
    # ------------------------------------------------------------------ #

    def _lib_symbol(self, lib_id: str) -> Node | None:
        lib = self.root.find("lib_symbols")
        for s in lib.find_all("symbol"):
            if s.items[1].value == lib_id:
                return s
        return None

    def _lib_pin(self, lib_id: str, number: str):
        if lib_id not in self._lib_pins_cache:
            pins: dict[str, tuple[float, float]] = {}
            ls = self._lib_symbol(lib_id)
            if ls is not None:
                for sub in ls.find_all("symbol"):
                    for pin in sub.find_all("pin"):
                        at = pin.find("at")
                        xy = (float(at.items[1].value), float(at.items[2].value))
                        pnum = pin.find("number")
                        if pnum:
                            pins[pnum.items[1].value] = xy
                        pname = pin.find("name")
                        if pname and pname.items[1].value not in ("~", ""):
                            pins.setdefault(pname.items[1].value, xy)
            self._lib_pins_cache[lib_id] = pins
        return self._lib_pins_cache[lib_id].get(number)

    def import_lib_symbol(self, kicad_sym_path, lib_id: str) -> None:
        """Copy a symbol definition from a `.kicad_sym` library into this sheet's
        `lib_symbols` cache (KiCad requires the definition to be present locally).
        The library `lib_id` (e.g. `Device:R_Small`) is rewritten to match the
        instance lib_id used on the sheet. No-op if already present."""
        if self._lib_symbol(lib_id) is not None:
            return
        libroot = sexpr.load(kicad_sym_path)
        bare = lib_id.split(":")[-1]
        src = None
        for s in libroot.find_all("symbol"):
            if s.items[1].value in (lib_id, bare):
                src = s
                break
        if src is None:
            raise KeyError(f"symbol {lib_id!r} not found in {kicad_sym_path}")
        # Re-key the symbol name to the on-sheet lib_id.
        src.items[1] = qstr(lib_id)
        self.root.find("lib_symbols").items.append(src)
        self._lib_pins_cache.pop(lib_id, None)

    # ------------------------------------------------------------------ #
    # Placement & connectivity
    # ------------------------------------------------------------------ #

    def place(self, lib_id: str, ref: str, at, value: str = "",
              rotation: int = 0, footprint: str = "", datasheet: str = "~",
              in_bom: bool = True, on_board: bool = True) -> PlacedSymbol:
        """Place a component instance. The lib symbol must already be present
        (placed before, migrated, or imported via `import_lib_symbol`)."""
        if self._lib_symbol(lib_id) is None:
            raise KeyError(
                f"lib symbol {lib_id!r} not in sheet; call import_lib_symbol() first"
            )
        x, y = float(at[0]), float(at[1])
        suid = self.reg.get(f"sym:{ref}")

        def prop(pname, pval, py_off, hide=False):
            eff = node("effects", node("font", node("size", num(1.27), num(1.27))))
            if hide:
                eff.items.append(node("hide", sym("yes")))
            return node("property", qstr(pname), qstr(pval),
                        node("at", num(x), num(_round(y + py_off)), num(0)), eff)

        snode = node(
            "symbol",
            node("lib_id", qstr(lib_id)),
            node("at", num(x), num(y), num(rotation)),
            node("unit", num(1)),
            node("exclude_from_sim", sym("no")),
            node("in_bom", sym("yes" if in_bom else "no")),
            node("on_board", sym("yes" if on_board else "no")),
            node("dnp", sym("no")),
            node("fields_autoplaced", sym("yes")),
            node("uuid", qstr(suid)),
            prop("Reference", ref, -3.81),
            prop("Value", value, 3.81),
            prop("Footprint", footprint, 0, hide=True),
            prop("Datasheet", datasheet, 0, hide=True),
            node("instances",
                 node("project", qstr(""),
                      node("path", qstr("/" + self.root_uuid),
                           node("reference", qstr(ref)), node("unit", num(1))))),
        )
        self._insert_before_tail(snode)
        return PlacedSymbol(self, snode, ref)

    def wire(self, a, b, key: str | None = None) -> Node:
        """Draw a wire between two coordinates (or two `pin()` results)."""
        ax, ay = float(a[0]), float(a[1])
        bx, by = float(b[0]), float(b[1])
        if key is None:
            key = f"wire:{min((ax, ay), (bx, by))}->{max((ax, ay), (bx, by))}"
        wuid = self.reg.get(key)
        wnode = node(
            "wire",
            node("pts", node("xy", num(ax), num(ay)), node("xy", num(bx), num(by))),
            node("stroke", node("width", num(0)), node("type", sym("default"))),
            node("uuid", qstr(wuid)),
        )
        self._insert_before_tail(wnode)
        return wnode

    def junction(self, at, key: str | None = None) -> Node:
        x, y = float(at[0]), float(at[1])
        juid = self.reg.get(key or f"junction:{x},{y}")
        jnode = node(
            "junction",
            node("at", num(x), num(y)),
            node("diameter", num(0)),
            node("color", num(0), num(0), num(0), num(0)),
            node("uuid", qstr(juid)),
        )
        self._insert_before_tail(jnode)
        return jnode

    def label(self, name: str, at, shape: str = "input", rotation: int = 0,
              key: str | None = None) -> Node:
        """Add a global net label at a coordinate. Matching label text = one net,
        which is the cleanest way to connect pins without drawing wires."""
        x, y = float(at[0]), float(at[1])
        luid = self.reg.get(key or f"glabel:{name}@{x},{y}")
        lnode = node(
            "global_label", qstr(name),
            node("shape", sym(shape)),
            node("at", num(x), num(y), num(rotation)),
            node("fields_autoplaced", sym("yes")),
            node("effects", node("font", node("size", num(1.27), num(1.27))),
                 node("justify", sym("left"))),
            node("uuid", qstr(luid)),
        )
        self._insert_before_tail(lnode)
        return lnode

    def net(self, name: str, pins, shape: str = "passive") -> None:
        """Connect a set of pins by dropping an identically-named global label at
        each pin coordinate. KiCad merges same-named labels into one net."""
        for i, p in enumerate(pins):
            xy = p.pin_xy if hasattr(p, "pin_xy") else p
            self.label(name, xy, shape=shape, key=f"glabel:{name}#{i}")

    # ------------------------------------------------------------------ #
    # Mutation of existing instances
    # ------------------------------------------------------------------ #

    def symbol(self, ref: str) -> PlacedSymbol:
        for s in self.root.find_all("symbol"):
            for p in s.find_all("property"):
                if p.items[1].value == "Reference" and p.items[2].value == ref:
                    return PlacedSymbol(self, s, ref)
        raise KeyError(f"no placed symbol with reference {ref!r}")

    def set_value(self, ref: str, value: str) -> None:
        sym_node = self.symbol(ref).node
        for p in sym_node.find_all("property"):
            if p.items[1].value == "Value":
                p.items[2] = qstr(value)
                return
        raise KeyError(f"{ref}: no Value property")

    # ------------------------------------------------------------------ #
    # internals
    # ------------------------------------------------------------------ #

    def _insert_before_tail(self, new_node: Node) -> None:
        """Insert before the trailing sheet_instances / embedded_fonts nodes so the
        file keeps KiCad's element-then-footer ordering."""
        items = self.root.items
        idx = len(items)
        for i, c in enumerate(items):
            if isinstance(c, Node) and c.tag in ("sheet_instances", "embedded_fonts"):
                idx = min(idx, i)
        items.insert(idx, new_node)
