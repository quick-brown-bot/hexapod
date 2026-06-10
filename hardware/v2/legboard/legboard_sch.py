"""Hexapod V2 — LegBoard schematic (source of truth).

Intelligent local actuator controller. One per leg. Owns *execution*: receives
motion targets over RS485, interpolates locally, generates the three servo PWM
signals, and reports telemetry. No IK / no body awareness here.

Contents:
* U1  XIAO RP2040 — local control loop (500-1000 Hz), PWM, watchdog, telemetry.
* U2  SP3485CN RS485 transceiver — bus slave node.
* J1  RJ11 (RJ25 6P used as 4-wire) uplink to MainBoard: NC / GND / RS485 A /
    RS485 B / +5V logic / NC.
* J2/J3/J4 servo connectors (coxa / femur / tibia): signal / +6V / GND.
* J5  servo-power input from the MainPowerBoard (+6V rail for this leg pair).
* J6  current/voltage sense module header (analog out -> RP2040 ADC).

Power model: +5V logic arrives over RJ11 and feeds the XIAO via VBUS; the XIAO's
on-board regulator provides +3.3V (its 3V3 pin) for the RS485 transceiver and
the sense module. Servo +6V arrives separately on J5 and never touches logic.

Run `python hardware/v2/legboard/legboard_sch.py` to (re)generate
`legboard.kicad_sch`. Connectivity is by net label.
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))  # hardware/v2
from _common import imp, power_flag  # noqa: E402

from hardware.schematic import Schematic, UuidRegistry  # noqa: E402

HERE = Path(__file__).resolve().parent


def build() -> Schematic:
    reg = UuidRegistry(HERE / "uuids.json")
    sch = Schematic.new("legboard", reg, paper="A4")

    imp(sch,
        "Seeed_Studio_XIAO_Series:XIAO-RP2040-DIP",
        "Hexapod_V2:SP3485CN",
        "Hexapod_V2:RJ25",
        "Connector:Conn_01x02_Pin",
        "Connector:Conn_01x03_Pin",
        "Connector:Conn_01x04_Pin",
        "Device:R_Small",
        "Device:C_Small",
        "Device:C_Polarized")

    # --- local controller ------------------------------------------------- #
    u1 = sch.place("Seeed_Studio_XIAO_Series:XIAO-RP2040-DIP", "U1", at=(120, 90),
                   value="XIAO-RP2040")
    sch.net("+5V", [u1.pin("14")])       # VBUS <- RJ11 logic power
    sch.net("+3.3V", [u1.pin("12")])     # 3V3 OUT (on-board regulator)
    sch.net("GND", [u1.pin("13")])
    sch.net("RP_TXD", [u1.pin("7")])     # P0/D6/TX -> RS485 DI
    sch.net("RP_RXD", [u1.pin("8")])     # P1/D7/RX <- RS485 RO
    sch.net("RS485_DE", [u1.pin("3")])   # P28/A2 -> driver enable
    sch.net("COXA_PWM", [u1.pin("11")])  # P3/D10
    sch.net("FEMUR_PWM", [u1.pin("10")])  # P4/D9
    sch.net("TIBIA_PWM", [u1.pin("9")])  # P2/D8
    sch.net("I_SENSE", [u1.pin("1")])    # P26/A0 (ADC)
    sch.net("V_SENSE", [u1.pin("2")])    # P27/A1 (ADC)

    # --- RS485 slave ------------------------------------------------------ #
    u2 = sch.place("Hexapod_V2:SP3485CN", "U2", at=(185, 90), value="SP3485CN")
    sch.net("+3.3V", [u2.pin("8")])      # VCC
    sch.net("GND", [u2.pin("5")])
    sch.net("RP_RXD", [u2.pin("1")])     # RO
    sch.net("RP_TXD", [u2.pin("4")])     # DI
    sch.net("RS485_DE", [u2.pin("2"), u2.pin("3")])  # ~RE + DE tied
    sch.net("RS485_A", [u2.pin("6")])
    sch.net("RS485_B", [u2.pin("7")])

    # Bus termination 120R — DNP except on the two electrical ends of the bus.
    rt = sch.place("Device:R_Small", "R1", at=(210, 90), value="120 (DNP)",
                   rotation=90)
    sch.net("RS485_A", [rt.pin("1")])
    sch.net("RS485_B", [rt.pin("2")])

    # --- RJ11 uplink: pins 1 and 6 intentionally unused ------------------ #
    j1 = sch.place("Hexapod_V2:RJ25", "J1", at=(50, 90), value="UPLINK_RJ11")
    sch.net("GND", [j1.pin("2")])
    sch.net("RS485_A", [j1.pin("3")])
    sch.net("RS485_B", [j1.pin("4")])
    sch.net("+5V", [j1.pin("5")])

    # --- servo power input ------------------------------------------------ #
    j5 = sch.place("Connector:Conn_01x02_Pin", "J5", at=(50, 140),
                   value="SERVO_PWR_IN")
    sch.net("+6V", [j5.pin("1")])
    sch.net("GND", [j5.pin("2")])

    # --- servo connectors (signal / +6V / GND) --------------------------- #
    for ref, x, pwm in (("J2", 150, "COXA_PWM"), ("J3", 175, "FEMUR_PWM"),
                        ("J4", 200, "TIBIA_PWM")):
        j = sch.place("Connector:Conn_01x03_Pin", ref, at=(x, 140), value=pwm[:-4])
        sch.net(pwm, [j.pin("1")])
        sch.net("+6V", [j.pin("2")])
        sch.net("GND", [j.pin("3")])

    # --- current / voltage sense module ---------------------------------- #
    j6 = sch.place("Connector:Conn_01x04_Pin", "J6", at=(90, 140), value="SENSE")
    sch.net("+3.3V", [j6.pin("1")])
    sch.net("GND", [j6.pin("2")])
    sch.net("I_SENSE", [j6.pin("3")])
    sch.net("V_SENSE", [j6.pin("4")])

    # --- decoupling / bulk ------------------------------------------------ #
    c1 = sch.place("Device:C_Small", "C1", at=(160, 70), value="100nF")
    sch.net("+3.3V", [c1.pin("1")])
    sch.net("GND", [c1.pin("2")])
    c2 = sch.place("Device:C_Polarized", "C2", at=(70, 140), value="470uF")
    sch.net("+6V", [c2.pin("1")])
    sch.net("GND", [c2.pin("2")])
    c3 = sch.place("Device:C_Small", "C3", at=(100, 70), value="100nF")
    sch.net("+5V", [c3.pin("1")])
    sch.net("GND", [c3.pin("2")])

    # --- ERC power flags -------------------------------------------------- #
    power_flag(sch, "#FLG1", (40, 160), "+5V")
    power_flag(sch, "#FLG2", (60, 160), "+3.3V")
    power_flag(sch, "#FLG3", (80, 160), "+6V")
    power_flag(sch, "#FLG4", (100, 160), "GND")

    return sch


if __name__ == "__main__":
    build().write(HERE / "legboard.kicad_sch")
    print("wrote legboard.kicad_sch")
