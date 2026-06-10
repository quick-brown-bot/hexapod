# Hexapod V1 boards

The original (Version 1) electrical design: a centralized controller that drives
the servos directly, before the V2 split into distributed leg controllers and a
separate power-distribution board. Kept as-is for reference and continued use.

This folder is self-contained — its own copy of the symbol libraries and
footprints lives alongside the designs, so the V1 boards do not depend on any
shared top-level libraries.

| Board | Files | Role |
|-------|-------|------|
| [servo_mainboard](servo_mainboard/) | `hexapod.kicad_*` | Central ESP32 board driving the servos directly. |
| [leg](leg/) | `leg_v2.kicad_*` | XIAO RP2040 leg controller (early distributed-leg experiment). |
| [esp32_connector](esp32_connector/) | `esp32_connector.kicad_*` | ESP32 carrier / breakout. |

## Layout

```
hardware/v1/
├── symbols/      ← Seeed XIAO + SymbolsLib (ESP32) symbol libraries
├── footprints/   ← Seeed XIAO footprints
├── models/       ← hexapod_v1.FCStd (mechanical model)
├── servo_mainboard/
├── leg/
└── esp32_connector/
```

The board project library tables (`sym-lib-table` / `fp-lib-table`) reference
these local copies via `${KIPRJMOD}`-relative paths. Stock KiCad symbols
(`Device:*`, `Connector:*`, `RF_Module:*`, `power:*`) resolve from the KiCad
install's global library table.

> V1 schematics are KiCad-native and edited in Eeschema. The schematic-as-code
> toolchain (`hardware/schematic/`) and the as-code workflow apply to the
> [V2 boards](../v2/README.md).
