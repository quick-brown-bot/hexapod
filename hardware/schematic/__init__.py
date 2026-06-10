"""Schematic-as-code toolchain for this repo's KiCad boards.

See .claude/skills/hardware-schematics/SKILL.md for the workflow and the rules
(UUID stability, documentation updates).
"""

from .registry import UuidRegistry
from .schematic import Schematic, PlacedSymbol
from . import sexpr

__all__ = ["UuidRegistry", "Schematic", "PlacedSymbol", "sexpr"]
