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

The physical cable is populated as a 4-wire link inside a 6P6C/RJ25 shell, so
pins 1 and 6 are intentionally left unconnected.

| Pin | Net | Function |
|-----|-----|----------|
| 1 | — | unused |
| 2 | GND | Ground |
| 3 | RS485_A | RS485 A |
| 4 | RS485_B | RS485 B |
| 5 | +5V | Logic power |
| 6 | — | unused |

## MainBoard nets

| Net | Connects |
|-----|----------|
| `+5V` | SBEC input (J7), 3V3-reg VIN (J8), all six RJ11 pin 5 |
| `+3.3V` | 3V3-reg out (J8), ESP32 V3.3, SP3485 VCC, IMU, decoupling, EN pull-up |
| `GND` | global ground |
| `ESP_TXD` / `ESP_RXD` | ESP32 IO17/IO16 (UART2) ↔ SP3485 DI/RO |
| `RS485_DE` | ESP32 IO4 → SP3485 DE + ~RE (direction) |
| `RS485_A` / `RS485_B` | SP3485 A/B ↔ all six RJ11, 120 Ω term (R1) |
| `IMU_SDA` / `IMU_SCL` / `IMU_INT` | ESP32 IO21/IO22/IO34 ↔ IMU header (J9) |

IMU header J9 (`Conn_01x05`): 1 = +3.3V, 2 = GND, 3 = SDA, 4 = SCL, 5 = INT.
3V3-reg module J8 (`Conn_01x03`): 1 = +5V in, 2 = GND, 3 = +3.3V out.

Footprints currently driven from the generator: `U1` uses
`Custom:ESP32_Dev_Board`, `J1..J6` use
`Connector_RJ:RJ25_Wayconn_MJEA-660X1_Horizontal`, `J9` uses
`Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical`, `U2` uses the
SOIC-8 SP3485 footprint, and the ordinary small R/C/LED parts default to 0805.

## LegBoard nets

| Net | Connects |
|-----|----------|
| `+5V` | RJ11 pin 5 logic power → XIAO VBUS (U1.14) |
| `+3.3V` | XIAO 3V3 out (U1.12) → SP3485 VCC, INA4181 VCC, decoupling |
| `+6V` | post-total-shunt servo bus; high-side **force** node feeding the three per-servo branch shunts and bulk cap C2 |
| `+6V_IN` | raw servo-power input (J5) ahead of the total shunt |
| `+6V_COXA` / `+6V_FEMUR` / `+6V_TIBIA` | post-branch-shunt servo supply rails (force) for J2 / J3 / J4 |
| `ITOT_SP` / `ITOT_SN` | Kelvin **sense** taps of total shunt R2 → INA IN+1 / IN-1 |
| `COXA_SP` / `COXA_SN` | Kelvin sense taps of coxa shunt R3 → INA IN+2 / IN-2 |
| `FEMUR_SP` / `FEMUR_SN` | Kelvin sense taps of femur shunt R4 → INA IN+3 / IN-3 |
| `TIBIA_SP` / `TIBIA_SN` | Kelvin sense taps of tibia shunt R5 → INA IN+4 / IN-4 |
| `GND` | global ground |
| `RP_TXD` / `RP_RXD` | XIAO P0/P1 (UART) ↔ SP3485 DI/RO |
| `RS485_DE` | XIAO P6/D4 → SP3485 DE + ~RE |
| `RS485_A` / `RS485_B` | SP3485 ↔ RJ11, 120 Ω term R1 (DNP except bus ends) |
| `COXA_PWM` / `FEMUR_PWM` / `TIBIA_PWM` | XIAO P3/P4/P2 → servo connectors J2/J3/J4 |
| `I_TOTAL_SENSE` | INA4181 channel 1 output → XIAO A0 |
| `I_COXA_SENSE` | INA4181 channel 2 output → XIAO A1 |
| `I_FEMUR_SENSE` | INA4181 channel 3 output → XIAO A2 |
| `I_TIBIA_SENSE` | INA4181 channel 4 output → XIAO A3 |

Servo connectors J2/J3/J4 (`Conn_01x03`): 1 = signal, 2 = branch +6V, 3 = GND.
J5 is a 2-pin screw terminal for servo power input. The four current shunts
`R2`/`R3`/`R4`/`R5` are **4-terminal (Kelvin) shunts** (`Device:R_Shunt`): pins
1/4 carry current, pins 2/3 are the sense taps. Each INA input reaches the shunt
over its own unique two-node `*_SP`/`*_SN` net, so the autorouter cannot merge
the three high-side taps (INA pins 6/15/17) through the shared `+6V` copper or
assign them current-rail trace widths. U3 (`INA4181A3IPWR`) measures channel 1
across the total-leg shunt `R2` (`+6V_IN`→`+6V`), channel 2 across coxa `R3`,
channel 3 across femur `R4`, channel 4 across tibia `R5`; IN+ is always on the
higher-potential (source) side. The generated sheet layout keeps the XIAO and INA
blocks as top-level anchors with the connector and power groups below for
readability.

> **Shunt placement must stay at rotation 0.** The schematic-as-code pin
> transform mismatches KiCad for off-axis pins at rotation 90/270 — the Kelvin
> sense pads land off the pin and the force pads swap. Verified via netlist
> export; keep the `Device:R_Shunt` placements un-rotated.

Footprints currently driven from the generator: `U1` uses
`Seeed_Studio_XIAO_Series:XIAO-RP2040-DIP`, `J1` uses
`Connector_RJ:RJ25_Wayconn_MJEA-660X1_Horizontal`, `U2` uses the SOIC-8
SP3485 footprint, `U3` uses `Custom:PW20_TEX`, `J2`/`J3`/`J4` use
`Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical`, `J5` uses
`TerminalBlock:TerminalBlock_MaiXu_MX126-5.0-02P_1x02_P5.00mm`, the four INA
shunts now use the 4-contact Kelvin footprint
`Resistor_SMD:R_Shunt_Ohmite_LVK12`, `C2` uses the through-hole radial candidate
`Capacitor_THT:CP_Radial_D8.0mm_P3.50mm` for an 8 mm diameter bulk capacitor, and
the ordinary small R/C parts default to 0805. Verify the LVK12 shunt power rating
and the capacitor lead pitch / body height against the exact parts before
freezing the PCB.

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

Footprints currently driven from the generator: the status resistor / LED parts
default to 0805. The bulk capacitors and the high-current reverse-polarity
Schottky are still explicit footprint-selection tasks rather than forced 0805
placeholders.

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
* **Remaining footprint work** is limited to the high-current / bulk power-path
  parts and the placeholder module headers; the ordinary V2 passives, ESP32,
  XIAO RP2040, RJ25 connectors, and the IMU header are now assigned in code.
