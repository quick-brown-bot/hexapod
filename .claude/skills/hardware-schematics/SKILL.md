---
name: hardware-schematics
description: >-
  Author and maintain this repo's KiCad electrical schematics as Python code —
  the boards under hardware/ (servo_mainboard, leg, esp32_connector) whose
  .kicad_sch files are generated from .py source. Use this skill whenever the
  user wants to define, edit, generate, or migrate a schematic / netlist /
  circuit / components / nets / connectors as code, set up or use the
  schematic-as-code DSL, regenerate a .kicad_sch from Python, or asks about
  keeping KiCad UUIDs stable so an existing PCB layout isn't lost. Prefer this
  code path over hand-editing a .kicad_sch, and trigger even when the user
  doesn't explicitly say "as code". NOT for PCB layout / routing / trace-width /
  copper pours in the pcb editor, BOM or netlist export, footprint or 3D-model
  design, running ERC in the GUI, firmware, or drawing diagram images — those
  are different tasks.
---

# Hardware schematics as code

This repo's electrical designs live under `hardware/` as KiCad projects
(`servo_mainboard/hexapod`, `leg/leg_v2`, `esp32_connector`). The goal of this
skill is to author those schematics from **Python source of truth** that
generates the `.kicad_sch` files, instead of editing them by hand in Eeschema.

The `.kicad_sch` format is plain S-expressions — no external library is needed,
pure Python reads and writes it.

## The non-negotiable rule: UUID stability preserves the PCB layout

A KiCad PCB references every schematic symbol by the path
`<root_schematic_uuid>/<symbol_uuid>`. If either UUID changes, KiCad treats the
footprint as a *new* part, drops its placement and routing, and you have to
re-lay-out the board. **The entire point of this toolchain is to never let that
happen.**

Therefore:

- The generator NEVER invents a UUID for an element it has seen before. It looks
  up a **stable key** (e.g. `sym:U1`, `wire:R1.2->D1.K`, `glabel:L_4_COXA@143.5,63.5`)
  in a committed `uuids.json` registry and reuses the stored UUID.
- Stable keys are derived from *identity* (reference designator, pin, net name),
  not from *position*. Moving a component on the canvas must not change its key.
- New elements get a fresh `uuid4()`, which is then written back to `uuids.json`.
- `uuids.json` is committed to git and **never hand-edited**. It is the memory
  that links code → schematic → board across regenerations.
- The root schematic `(uuid ...)` is itself stored in the registry under a key
  like `root:leg_v2` and reused on every run.

See `references/uuid-registry.md` for the registry design and key scheme, and
`references/kicad-sch-format.md` for the exact S-expression shapes (symbol
instance, wire, junction, label, the `instances` block, `sheet_instances`).

## ⚠️ Always update documentation in the same change

Every time you touch a schematic through this skill — add a component, change a
net, migrate a board, or extend the toolchain — **update the documentation in
the same commit**. Schematic-as-code is only maintainable if the docs track it.
This is a standing requirement, not an optional nicety. Specifically:

- Update the board's own notes (a `README.md` next to the `.kicad_sch`, or the
  relevant file under `docs/`) describing what changed and why.
- Keep the connector / pin-mapping / net tables current — these are the parts
  humans actually read, and stale pin tables cause wiring mistakes.
- If you added or changed a DSL primitive or a stable-key convention, document
  it in `references/uuid-registry.md` and the generator's module docstring.
- Note the change in the project changelog / commit message in human terms
  ("added 470R + LED status indicator on IO2"), not just "regenerated sch".

If you finish a schematic change without updating docs, the task is not done.

## File layout per board

```
hardware/<board>/
├── <board>_sch.py     ← source of truth — you EDIT this
├── uuids.json         ← committed registry — generated, never hand-edited
└── <board>.kicad_sch  ← generated output — open in KiCad to lay out the PCB
```

Shared generator/DSL code lives once at `hardware/schematic/` (the
`schematic.py` library and `migrate.py` tool), imported by each board script.

## Workflow

### Authoring or editing a schematic

1. Read the board's `<board>_sch.py`. Make the change in Python inside `build()`
   (`sch.place("lib_id", "ref", at=(x, y), value=...)`, `sch.net(name, [pins])`,
   `sch.wire(a, b)`, `sch.set_value("R3", "1k")`). Import a new part's symbol with
   `sch.import_lib_symbol(...)` before placing it.
2. Prefer **net labels** over explicit wires where possible — connecting by net
   name avoids hand-computing wire endpoints and is how KiCad nets work anyway.
3. Run the script to regenerate the `.kicad_sch`. Confirm `uuids.json` only
   *grew* (new keys) and did not rewrite existing UUIDs — `git diff uuids.json`
   should show additions, not modifications, for unchanged parts.
4. Open in KiCad, run ERC, update the PCB from schematic. Existing placements
   must be preserved; only genuinely new parts appear unplaced.
5. **Update the docs** (see the rule above). Then commit `*_sch.py`,
   `uuids.json`, the `.kicad_sch`, and the doc changes together.

### Creating a new board from scratch

Use this when there is no existing `.kicad_sch` to start from. The Python script
is the *sole* source of truth — there is no frozen `.base` file (that only exists
for migrated boards), so the script builds the whole schematic from primitives.

```python
from hardware.schematic import Schematic, UuidRegistry

reg = UuidRegistry("hardware/<board>/uuids.json")
sch = Schematic.new("<board>", reg, paper="A4")   # root uuid comes from registry

# Every part needs its symbol definition imported from a .kicad_sym library once;
# unlike a migrated board, a new sheet starts with an empty lib_symbols cache.
sch.import_lib_symbol("hardware/symbols/Seeed_Studio_XIAO_Series.kicad_sym",
                      "Seeed_Studio_XIAO_Series:XIAO-RP2040-DIP")
u1 = sch.place("Seeed_Studio_XIAO_Series:XIAO-RP2040-DIP", "U1", at=(100, 100))

sch.import_lib_symbol(R_SMALL_LIB, "Device:R_Small")
r1 = sch.place("Device:R_Small", "R1", at=(80, 90), value="470", rotation=90)

sch.net("LED_OUT", [u1.pin("IO2"), r1.pin("1")])   # connect by net label

sch.write("hardware/<board>/<board>.kicad_sch")
```

Key differences from a migrated board:

- **No `.base` file.** Source of truth is just `<board>_sch.py` + `uuids.json`.
  Re-running rebuilds the sheet from scratch, so it is naturally idempotent.
- **Import every symbol.** `import_lib_symbol(<.kicad_sym>, <lib_id>)` must run
  before the first `place()` of each new part type. In-repo libraries live at
  `hardware/symbols/` and `hardware/SymbolsLib.kicad_sym`; stock KiCad symbols
  (`Device:*`, `power:*`) come from the KiCad install's libraries.
- **UUID stability still applies** the moment you first lay out the PCB: from then
  on, `uuids.json` is what protects the layout, exactly as for a migrated board.

You also need a matching `<board>.kicad_pro` for KiCad to open the project — copy
one from an existing board and update the name.

### Migrating an existing schematic into code (one-time per board)

```bash
python -m hardware.schematic.migrate hardware/leg/leg_v2.kicad_sch
```

The migrator freezes the original as `<board>.base.kicad_sch`, **seeds
`uuids.json`** with the root UUID and every component UUID (keyed by reference
designator), and emits `<board>_sch.py` (which loads the frozen base, applies your
edits, and writes the working `.kicad_sch`). Because the engine round-trips
untouched trees byte-for-byte, a freshly migrated board regenerates with **no UUID
churn** — verify with `git diff`. The boards under `hardware/` (`leg_v2`,
`esp32_connector`, `hexapod`) are not migrated yet; when migrating one, start with
the smallest and confirm the byte-exact round-trip before trusting it on a large
board. Commit a board's migration (`<board>_sch.py`, `<board>.base.kicad_sch`,
`uuids.json`) only once you've verified its PCB footprint links still resolve.

## The toolchain

`hardware/schematic/` already exists (see its `README.md`):

- `sexpr.py` — lossless S-expression parser + KiCad-faithful serializer. Round-trips
  every board byte-for-byte; preserving each atom's source text is what keeps diffs
  (and UUIDs) clean.
- `registry.py` — `UuidRegistry` (load/lookup/seed/save `uuids.json`).
- `schematic.py` — the `Schematic` DSL: `new`/`load`, `place`, `wire`, `junction`,
  `label`, `net`, `set_value`, and pin world-coordinate resolution for all four
  rotations.
- `migrate.py` — freezes a board, seeds the registry, emits the source-of-truth script.

When **extending** it (new primitive, new format element), match
`references/kicad-sch-format.md` exactly — KiCad is strict about the `lib_symbols`
library, the per-symbol `instances` block, and the trailing `sheet_instances` /
`embedded_fonts` entries. After any change to `sexpr.py`, re-run the byte-exact
round-trip check on all three boards before trusting it.

## What this toolchain does NOT do

- **Auto-routing / auto-placement of wires** — you specify wire paths, or (better)
  connect by net label and skip wires entirely.
- **Inventing pin positions for unknown symbols** — a new symbol needs its pin
  data supplied or pulled from a `.kicad_sym` library.
- **Hierarchical sheets** — single-sheet only for now; add later if needed.

When you hit one of these, say so plainly rather than guessing.
