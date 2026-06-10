# `hardware/schematic` — schematics as code

Author and maintain the KiCad schematics in `hardware/` from Python instead of
hand-editing `.kicad_sch` files in Eeschema. The point is **UUID stability**: the
PCB references each symbol by `<root_uuid>/<symbol_uuid>`, so as long as those
UUIDs are preserved across regenerations, your board layout (placement + routing)
is never lost. A committed `uuids.json` per board is what guarantees that.

> Update the board's documentation in the same change as any schematic edit. See
> `.claude/skills/hardware-schematics/SKILL.md` for the full workflow and rules.

## Modules

| File | What it is |
|------|------------|
| `sexpr.py` | Lossless S-expression parser + KiCad-faithful serializer. Round-trips a board's `.kicad_sch` byte-for-byte. |
| `registry.py` | `UuidRegistry` — maps stable keys (`sym:U1`, `root:leg_v2`, …) to UUIDs in `uuids.json`. Committed, never hand-edited. |
| `schematic.py` | `Schematic` DSL — load/new, `place`, `wire`, `junction`, `label`, `net`, `set_value`. Resolves pin world-coordinates for all symbol rotations. |
| `migrate.py` | One-time per board: freezes the original, seeds the registry, emits the source-of-truth script. |

## Per-board files (after migration)

```
hardware/<board>/
├── <board>_sch.py        ← source of truth — EDIT this
├── <board>.base.kicad_sch ← frozen migration capture — do not edit / open in KiCad
├── uuids.json            ← committed UUID registry — never hand-edit
└── <board>.kicad_sch     ← generated — open THIS in KiCad to lay out the PCB
```

The script loads the frozen `.base`, applies your Python edits, and writes the
working `.kicad_sch`. Loading an immutable base (not the file it writes) keeps
regeneration deterministic and idempotent.

## Workflow

Migrate a board (the boards under `hardware/` are not migrated yet):

```bash
python -m hardware.schematic.migrate hardware/leg/leg_v2.kicad_sch
```

Edit and regenerate:

```bash
# edit hardware/leg/leg_v2_sch.py, then:
python hardware/leg/leg_v2_sch.py
git diff hardware/leg/uuids.json     # additions only — never modified values
```

Then in KiCad: open the regenerated `.kicad_sch`, run ERC, *Update PCB from
Schematic*. Existing parts keep their placement; only new parts appear unplaced.

## Example edit

```python
def build():
    reg = UuidRegistry(HERE / "uuids.json")
    sch = Schematic.load(HERE / "leg_v2.base.kicad_sch", reg, name="leg_v2")

    # add a status LED off the RP2040; connect by net label (no wires needed)
    sch.import_lib_symbol(R_SMALL_LIB, "Device:R_Small")
    sch.import_lib_symbol(LED_LIB, "Device:LED_Small")
    r = sch.place("Device:R_Small",   "R10", at=(60, 50), value="470", rotation=90)
    d = sch.place("Device:LED_Small", "D10", at=(60, 60))
    sch.net("STATUS", [r.pin("2"), d.pin("A")])

    return sch
```

`place()` requires the symbol definition to be present in the sheet; for a part not
already used on the board, `import_lib_symbol(<.kicad_sym path>, <lib_id>)` copies it
from a KiCad symbol library first.

## Guarantees (and what's verified)

- **Byte-exact round-trip** of all three existing boards (`sexpr`), so migration
  introduces **zero** UUID churn.
- **Pin geometry** validated against real wire endpoints for rotations 0/90/180/270.
- **Idempotent** regeneration: running a board script twice yields identical output.

## Limits

- No auto-routing/placement — give wire coordinates or (better) connect by net label.
- New symbol types need their pin data imported from a `.kicad_sym` library.
- Single-sheet only (no hierarchical sheets yet).
- Once migrated, make schematic changes in Python, not in Eeschema — direct
  Eeschema edits to the schematic would be overwritten on the next regenerate.
  (PCB layout is separate and is preserved.)
