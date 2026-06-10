# Hexapod V2 boards

Schematics for the [V2 system architecture](../../docs/architecture/HEXAPOD_V2_SYSTEM_ARCHITECTURE.md):
distributed leg controllers with logic power separated from servo power. V1
(`hardware/servo_mainboard`, `hardware/leg`, `hardware/esp32_connector`) is left
untouched — V2 lives here in parallel.

Three boards:

| Board | Source | Role |
|-------|--------|------|
| [MainBoard](mainboard/) | [`mainboard_sch.py`](mainboard/mainboard_sch.py) | ESP32 + IMU + RS485 master + 6× RJ11. Logic & comms only. |
| [LegBoard](legboard/) | [`legboard_sch.py`](legboard/legboard_sch.py) | XIAO RP2040 + SP3485 + 3 servos + sensing. One per leg. |
| [MainPowerBoard](powerboard/) | [`powerboard_sch.py`](powerboard/powerboard_sch.py) | Battery → fuse/switch/protection → 3 UBECs + SBEC → legs. |

These schematics are authored **as code** with the
[`hardware-schematics`](../schematic/README.md) toolchain. The Python script is
the source of truth; `uuids.json` is the committed UUID registry that protects
the PCB layout across regenerations. Edit the `.py`, never the `.kicad_sch` by
hand.

## Regenerating

```bash
python hardware/v2/mainboard/mainboard_sch.py
python hardware/v2/legboard/legboard_sch.py
python hardware/v2/powerboard/powerboard_sch.py
```

Each writes its `<board>.kicad_sch` and updates `<board>/uuids.json` (additions
only for unchanged parts). `hardware/v2/_common.py` resolves symbol libraries:
stock KiCad libs from the install (override with `KICAD_SYMBOL_DIR`), plus the
v2-local libraries under [`symbols/`](symbols/) — the Seeed XIAO series
(`Seeed_Studio_XIAO_Series.kicad_sym`), the ESP32 module (`SymbolsLib.kicad_sym`),
and `Hexapod_V2.kicad_sym` (flattened standalone `RJ25` and `SP3485CN`, because
the stock library defines those as derived `(extends …)` symbols the code
importer cannot follow).

V2 is self-contained: its own copy of the symbol libraries, footprints, and the
v2 FreeCAD model live here under [`symbols/`](symbols/), [`footprints/`](footprints/)
and [`models/`](models/); the boards do not depend on shared top-level libraries.

## Inter-board connections

```text
MainPowerBoard.J_LOGIC (+5V) ──► MainBoard.J7 (SBEC_5V_IN)
MainBoard.J1..J6 (RJ11)      ──► LegBoard.J1 (UPLINK_RJ11)        ×6
MainPowerBoard.J1..J6 (+6V)  ──► LegBoard.J5 (SERVO_PWR_IN)       ×6
```

### RJ11 / RJ25 leg link (MainBoard ↔ LegBoard)

| Pin | Net | Function |
|-----|-----|----------|
| 1 | GND | Ground |
| 2 | +5V | Logic power |
| 3 | RS485_A | RS485 A |
| 4 | RS485_B | RS485 B |
| 5,6 | — | unused |

## MainBoard nets

| Net | Connects |
|-----|----------|
| `+5V` | SBEC input (J7), 3V3-reg VIN (J8), all six RJ11 pin 2 |
| `+3.3V` | 3V3-reg out (J8), ESP32 V3.3, SP3485 VCC, IMU, decoupling, EN pull-up |
| `GND` | global ground |
| `ESP_TXD` / `ESP_RXD` | ESP32 IO17/IO16 (UART2) ↔ SP3485 DI/RO |
| `RS485_DE` | ESP32 IO4 → SP3485 DE + ~RE (direction) |
| `RS485_A` / `RS485_B` | SP3485 A/B ↔ all six RJ11, 120 Ω term (R1) |
| `IMU_SDA` / `IMU_SCL` / `IMU_INT` | ESP32 IO21/IO22/IO34 ↔ IMU header (J9) |

IMU header J9 (`Conn_01x05`): 1 = +3.3V, 2 = GND, 3 = SDA, 4 = SCL, 5 = INT.
3V3-reg module J8 (`Conn_01x03`): 1 = +5V in, 2 = GND, 3 = +3.3V out.

## LegBoard nets

| Net | Connects |
|-----|----------|
| `+5V` | RJ11 logic power → XIAO VBUS (U1.14) |
| `+3.3V` | XIAO 3V3 out (U1.12) → SP3485 VCC, sense header |
| `+6V` | servo-power in (J5) → 3 servo connectors, bulk cap C2 |
| `GND` | global ground |
| `RP_TXD` / `RP_RXD` | XIAO P0/P1 (UART) ↔ SP3485 DI/RO |
| `RS485_DE` | XIAO P28 → SP3485 DE + ~RE |
| `RS485_A` / `RS485_B` | SP3485 ↔ RJ11, 120 Ω term R1 (DNP except bus ends) |
| `COXA_PWM` / `FEMUR_PWM` / `TIBIA_PWM` | XIAO P3/P4/P2 → servo connectors J2/J3/J4 |
| `I_SENSE` / `V_SENSE` | sense header (J6) → XIAO A0/A1 |

Servo connectors J2/J3/J4 (`Conn_01x03`): 1 = signal, 2 = +6V, 3 = GND.
Sense header J6 (`Conn_01x04`): 1 = +3.3V, 2 = GND, 3 = I_SENSE, 4 = V_SENSE.

## MainPowerBoard nets

| Net | Connects |
|-----|----------|
| `BATT_RAW`→`BATT_FUSED`→`BATT_SW`→`+VMAIN` | battery → F1 fuse → SW1 master switch → D1 reverse-polarity → main rail |
| `+VMAIN` | feeds the three UBEC modules (J8/J9/J10), the SBEC (J11), monitor (J13), bulk C4 |
| `VSERVO1` | UBEC1 out → legs 1,2 (J1,J2), bulk C1, LED D2 |
| `VSERVO2` | UBEC2 out → legs 3,4 (J3,J4), bulk C2, LED D3 |
| `VSERVO3` | UBEC3 out → legs 5,6 (J5,J6), bulk C3, LED D4 |
| `+5V` | SBEC out (J11) → logic output (J12), LED D5 |
| `GND` | global ground |

UBEC/SBEC module headers (`Conn_01x04`): 1 = +VMAIN in, 2 = GND, 3 = regulated
out, 4 = GND. Monitor header J13 (`Conn_01x05`): +VMAIN, VSERVO1, VSERVO2,
VSERVO3, GND.

## Status / known limitations (first pass)

These are deliberate module-abstraction simplifications to refine in Eeschema
before fabrication:

* **Reverse-polarity protection** is a placeholder series Schottky (D1). Replace
  with an ideal-diode / P-MOSFET circuit — a series diode wastes too much at
  servo currents.
* **UBEC / SBEC / IMU / current sensor** are represented as module headers, not
  discrete designs.
* **ERC** loads cleanly but reports expected violations: unused ESP32/XIAO GPIO
  and the ESP32 `VIN`/`SENSOR_*` pins are left open (add no-connect flags), and
  off-grid endpoints (the loose auto-layout is meant to be re-placed in
  Eeschema). Net-label connectivity is verified correct via exported netlist.
* **Footprints** are not yet assigned for the discrete passives/connectors.
