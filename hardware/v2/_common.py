"""Shared helpers for the Hexapod V2 board schematic scripts.

Every V2 board (`mainboard`, `legboard`, `powerboard`) is authored as code with
the `hardware.schematic` DSL. This module centralises the two things they all
need:

* **Symbol sourcing.** KiCad isn't on every machine at the same path, and a few
  symbols live in-repo (the Seeed XIAO series, the ESP32-WROOM module). `imp()`
  resolves a `lib_id` to the right `.kicad_sym` file and imports it once.
* **Boilerplate** — `repo_root()` so a board script can be run directly
  (`python hardware/v2/mainboard/mainboard_sch.py`) with `hardware` importable,
  and `power_flag()` to drop a `PWR_FLAG` on a source rail for clean ERC.

Net-label connectivity is the rule on these boards: components are placed on a
loose grid and joined with `sch.net(name, [pins])`. Exact placement only affects
readability — the netlist comes from matching global-label names — so the layout
here is deliberately simple and is meant to be tidied in Eeschema.
"""

from __future__ import annotations

import os
import sys
from pathlib import Path


def repo_root() -> Path:
    """Repo root (the dir containing the top-level `hardware/` package)."""
    # this file is <root>/hardware/v2/_common.py
    return Path(__file__).resolve().parents[2]


# Make `import hardware.schematic` work when a board script is run as __main__.
sys.path.insert(0, str(repo_root()))

# V2 keeps its own copy of the symbol libraries under hardware/v2/symbols, so the
# boards are self-contained and don't depend on shared top-level libraries.
V2_DIR = Path(__file__).resolve().parent
REPO_SYMBOLS = V2_DIR / "symbols"
SYMBOLSLIB = V2_DIR / "symbols" / "SymbolsLib.kicad_sym"


def _kicad_symbol_dir() -> Path:
    """Locate the installed KiCad stock symbol libraries.

    Honours `KICAD9_SYMBOL_DIR` / `KICAD_SYMBOL_DIR` first, then probes the
    common Windows / Linux install locations. Raise a clear error if none hit so
    the failure is actionable rather than a mysterious missing-symbol KeyError.
    """
    env = os.environ.get("KICAD9_SYMBOL_DIR") or os.environ.get("KICAD_SYMBOL_DIR")
    candidates = []
    if env:
        candidates.append(Path(env))
    local = os.environ.get("LOCALAPPDATA", "")
    if local:
        candidates.append(Path(local) / "Programs/KiCad/9.0/share/kicad/symbols")
    candidates += [
        Path(r"C:/Users/user/AppData/Local/Programs/KiCad/9.0/share/kicad/symbols"),
        Path(r"C:/Program Files/KiCad/9.0/share/kicad/symbols"),
        Path("/usr/share/kicad/symbols"),
        Path("/Applications/KiCad/KiCad.app/Contents/SharedSupport/symbols"),
    ]
    for c in candidates:
        if c.exists():
            return c
    raise FileNotFoundError(
        "KiCad stock symbol libraries not found. Set KICAD_SYMBOL_DIR to your "
        "KiCad 9 'share/kicad/symbols' directory."
    )


KICAD_SYMS = _kicad_symbol_dir()


def _lib_file(lib_id: str) -> Path:
    """Map a `Lib:Symbol` id to the `.kicad_sym` file that defines it."""
    lib = lib_id.split(":")[0]
    if lib == "Seeed_Studio_XIAO_Series":
        return REPO_SYMBOLS / "Seeed_Studio_XIAO_Series.kicad_sym"
    if lib == "Hexapod_V2":
        # In-repo flattened parts (RJ25, SP3485CN) — the stock library defines
        # these as derived `(extends ...)` symbols, which the DSL importer can't
        # follow, so we keep self-contained standalone copies harvested from V1.
        return REPO_SYMBOLS / "Hexapod_V2.kicad_sym"
    if lib == "SymbolsLib":
        return SYMBOLSLIB
    return KICAD_SYMS / f"{lib}.kicad_sym"


def imp(sch, *lib_ids: str) -> None:
    """Import one or more library symbols into `sch` (no-op if already present)."""
    for lib_id in lib_ids:
        sch.import_lib_symbol(str(_lib_file(lib_id)), lib_id)


def power_flag(sch, ref: str, at, net: str):
    """Place a `PWR_FLAG` and tie it to `net` so KiCad ERC sees the rail driven."""
    imp(sch, "power:PWR_FLAG")
    pf = sch.place("power:PWR_FLAG", ref, at=at, value="PWR_FLAG")
    sch.net(net, [pf.pin("1")])
    return pf
