"""Hexapod V2 — MainPowerBoard schematic (source of truth).

Owns all high-current energy distribution. Servo current must NEVER flow through
the MainBoard; it flows battery -> protection -> UBECs -> here -> LegBoards.

Contents:
* J_BATT  battery input (screw terminal) -> fuse -> master switch -> reverse-
  polarity protection -> +VMAIN distribution rail.
* J_UBEC1/2/3  servo BEC module headers (battery in, +6V out). UBEC#1 -> legs 1-2,
  UBEC#2 -> legs 3-4, UBEC#3 -> legs 5-6 (three independent VSERVO rails).
* J_SBEC  logic BEC module header (battery in, +5V out) -> MainBoard.
* J_LEG1..6  servo-power outputs to the six LegBoards.
* J_LOGIC  +5V logic output to the MainBoard.
* C1..C4  bulk capacitance (per-UBEC + battery rail) for transient servo loads.
* D2..D5  power-present status LEDs (UBEC1/2/3 + SBEC).
* J_MON   monitoring header: battery + per-UBEC voltages to MainBoard telemetry.

Protection (first pass): F1 main fuse, SW1 master-switch header, and D1 a series
Schottky standing in for reverse-polarity protection — replace with an ideal-
diode / P-MOSFET circuit before fabrication (a series diode wastes too much at
servo currents).

Run `python hardware/v2/powerboard/powerboard_sch.py` to (re)generate
`powerboard.kicad_sch`. Connectivity is by net label.
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))  # hardware/v2
from _common import LED_0805_FOOTPRINT, RES_0805_FOOTPRINT, imp, power_flag  # noqa: E402

from hardware.schematic import Schematic, UuidRegistry  # noqa: E402

HERE = Path(__file__).resolve().parent

# UBEC -> leg allocation and the VSERVO rail each leg pair runs on.
LEG_RAIL = {1: "VSERVO1", 2: "VSERVO1", 3: "VSERVO2",
            4: "VSERVO2", 5: "VSERVO3", 6: "VSERVO3"}


def build() -> Schematic:
    reg = UuidRegistry(HERE / "uuids.json")
    sch = Schematic.new("powerboard", reg, paper="A4")

    imp(sch,
        "Connector:Screw_Terminal_01x02",
        "Connector:Conn_01x02_Pin",
        "Connector:Conn_01x04_Pin",
        "Connector:Conn_01x05_Pin",
        "Device:Fuse",
        "Device:D_Schottky",
        "Device:R_Small",
        "Device:C_Polarized",
        "Device:LED_Small")

    # --- battery input & protection chain -------------------------------- #
    j_bat = sch.place("Connector:Screw_Terminal_01x02", "J7", at=(40, 60),
                      value="BATTERY")
    sch.net("BATT_RAW", [j_bat.pin("1")])
    sch.net("GND", [j_bat.pin("2")])

    f1 = sch.place("Device:Fuse", "F1", at=(60, 50), value="20A", rotation=90)
    sch.net("BATT_RAW", [f1.pin("1")])
    sch.net("BATT_FUSED", [f1.pin("2")])

    sw1 = sch.place("Connector:Conn_01x02_Pin", "SW1", at=(80, 55),
                    value="MASTER_SW")
    sch.net("BATT_FUSED", [sw1.pin("1")])
    sch.net("BATT_SW", [sw1.pin("2")])

    # Series Schottky = first-pass reverse-polarity protection (anode=pin2).
    d1 = sch.place("Device:D_Schottky", "D1", at=(100, 50), value="REV_PROT",
                   rotation=180)
    sch.net("BATT_SW", [d1.pin("2")])
    sch.net("+VMAIN", [d1.pin("1")])

    c4 = sch.place("Device:C_Polarized", "C4", at=(115, 55), value="1000uF")
    sch.net("+VMAIN", [c4.pin("1")])
    sch.net("GND", [c4.pin("2")])

    # --- servo UBEC modules (battery in -> VSERVOx out) ------------------ #
    for n, x in ((1, 50), (2, 90), (3, 130)):
        ju = sch.place("Connector:Conn_01x04_Pin", f"J{7 + n}", at=(x, 100),
                       value=f"UBEC{n}")
        rail = f"VSERVO{n}"
        sch.net("+VMAIN", [ju.pin("1")])   # battery in +
        sch.net("GND", [ju.pin("2")])
        sch.net(rail, [ju.pin("3")])       # regulated servo out
        sch.net("GND", [ju.pin("4")])
        # bulk capacitor on each servo rail
        c = sch.place("Device:C_Polarized", f"C{n}", at=(x + 20, 105),
                      value="2200uF")
        sch.net(rail, [c.pin("1")])
        sch.net("GND", [c.pin("2")])
        # power-present status LED
        rl = sch.place("Device:R_Small", f"R{n}", at=(x + 10, 75), value="1k",
                       rotation=90, footprint=RES_0805_FOOTPRINT)
        dl = sch.place("Device:LED_Small", f"D{1 + n}", at=(x + 10, 88),
                       value=f"UBEC{n}", rotation=90,
                       footprint=LED_0805_FOOTPRINT)
        sch.net(rail, [rl.pin("1")])
        sch.net(f"LED_U{n}", [rl.pin("2"), dl.pin("2")])
        sch.net("GND", [dl.pin("1")])

    # --- logic SBEC module (battery in -> +5V out) ----------------------- #
    j_sbec = sch.place("Connector:Conn_01x04_Pin", "J11", at=(170, 100),
                       value="SBEC")
    sch.net("+VMAIN", [j_sbec.pin("1")])
    sch.net("GND", [j_sbec.pin("2")])
    sch.net("+5V", [j_sbec.pin("3")])
    sch.net("GND", [j_sbec.pin("4")])
    r4 = sch.place("Device:R_Small", "R4", at=(180, 75), value="1k",
                   rotation=90, footprint=RES_0805_FOOTPRINT)
    d5 = sch.place("Device:LED_Small", "D5", at=(180, 88), value="SBEC",
                   rotation=90, footprint=LED_0805_FOOTPRINT)
    sch.net("+5V", [r4.pin("1")])
    sch.net("LED_SBEC", [r4.pin("2"), d5.pin("2")])
    sch.net("GND", [d5.pin("1")])

    # --- logic output to MainBoard --------------------------------------- #
    j_logic = sch.place("Connector:Conn_01x02_Pin", "J12", at=(200, 100),
                        value="LOGIC_5V_OUT")
    sch.net("+5V", [j_logic.pin("1")])
    sch.net("GND", [j_logic.pin("2")])

    # --- servo-power outputs to the six legs ----------------------------- #
    for leg in range(1, 7):
        jl = sch.place("Connector:Conn_01x02_Pin", f"J{leg}", at=(40 + (leg - 1) * 25, 150),
                       value=f"LEG{leg}_PWR")
        sch.net(LEG_RAIL[leg], [jl.pin("1")])
        sch.net("GND", [jl.pin("2")])

    # --- monitoring header ----------------------------------------------- #
    j_mon = sch.place("Connector:Conn_01x05_Pin", "J13", at=(210, 150),
                      value="MONITOR")
    sch.net("+VMAIN", [j_mon.pin("1")])
    sch.net("VSERVO1", [j_mon.pin("2")])
    sch.net("VSERVO2", [j_mon.pin("3")])
    sch.net("VSERVO3", [j_mon.pin("4")])
    sch.net("GND", [j_mon.pin("5")])

    # --- ERC power flags (battery source + module outputs) --------------- #
    power_flag(sch, "#FLG1", (40, 75), "BATT_RAW")
    power_flag(sch, "#FLG2", (60, 120), "VSERVO1")
    power_flag(sch, "#FLG3", (100, 120), "VSERVO2")
    power_flag(sch, "#FLG4", (140, 120), "VSERVO3")
    power_flag(sch, "#FLG5", (170, 120), "+5V")
    power_flag(sch, "#FLG6", (190, 60), "+VMAIN")
    power_flag(sch, "#FLG7", (40, 170), "GND")

    return sch


if __name__ == "__main__":
    build().write(HERE / "powerboard.kicad_sch")
    print("wrote powerboard.kicad_sch")
