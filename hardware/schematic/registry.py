"""Stable UUID registry.

`uuids.json` is the durable link between the Python source, the generated
`.kicad_sch`, and the laid-out PCB. KiCad references every schematic symbol from
the board by the path `<root_uuid>/<symbol_uuid>`; if either changes, KiCad treats
the footprint as a brand-new part and discards its placement and routing. The
registry exists so we never let that happen: every UUID we emit is looked up by a
*stable key* derived from identity (reference designator, net name, pin pair) and
reused on every regeneration.

The file is committed to git and never hand-edited. A normal edit only *adds*
keys — a changed value for an existing key means a key collided or was recomputed,
and that element's board link just broke. See references/uuid-registry.md.
"""

from __future__ import annotations

import json
import uuid as _uuid
from pathlib import Path


class UuidRegistry:
    def __init__(self, path):
        self.path = Path(path)
        self._map: dict[str, str] = {}
        self._dirty = False
        if self.path.exists():
            self._map = json.loads(self.path.read_text(encoding="utf-8"))

    def get(self, key: str) -> str:
        """Return the UUID for `key`, minting and storing a fresh one if new."""
        if key not in self._map:
            self._map[key] = str(_uuid.uuid4())
            self._dirty = True
        return self._map[key]

    def seed(self, key: str, value: str) -> None:
        """Record a known UUID (used by the migrator to capture existing ids).

        Refuses to silently overwrite an existing mapping with a different value —
        that would mean two elements claimed the same stable key, which is exactly
        the bug the registry is meant to prevent.
        """
        existing = self._map.get(key)
        if existing is not None and existing != value:
            raise ValueError(
                f"stable key {key!r} already maps to {existing}, refusing to "
                f"reassign to {value}; fix the key derivation"
            )
        if existing is None:
            self._map[key] = value
            self._dirty = True

    def has(self, key: str) -> bool:
        return key in self._map

    def save(self) -> None:
        """Write back with sorted keys so diffs stay readable and merge-friendly."""
        text = json.dumps(self._map, indent=2, sort_keys=True, ensure_ascii=False)
        self.path.write_text(text + "\n", encoding="utf-8")
        self._dirty = False

    def __len__(self):
        return len(self._map)
