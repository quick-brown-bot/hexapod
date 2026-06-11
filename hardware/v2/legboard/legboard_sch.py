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
* J5  servo-power input screw terminal from the MainPowerBoard (+6V rail for
    this leg pair).
* U3  INA4181A3IPWR quad current-sense amplifier; channel 1 monitors total leg
    current, channels 2-4 monitor coxa / femur / tibia branch currents.
* R2/R3/R4/R5  10 mOhm shunts for total leg current and each servo branch.
* NT1..NT5  net ties that split the heavy-current +6V rails from thin INA sense
    routes.

Power model: +5V logic arrives over RJ11 and feeds the XIAO via VBUS; the XIAO's
on-board regulator provides +3.3V (its 3V3 pin) for the RS485 transceiver and
the INA4181. Servo +6V arrives separately on J5; one shunt measures total leg
current and three more shunts measure the individual servo branches.

Run `python hardware/v2/legboard/legboard_sch.py` to (re)generate
`legboard.kicad_sch`. Connectivity is by net label.
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))  # hardware/v2
from _common import (  # noqa: E402
    BULK_CAP_470UF_FOOTPRINT,
    CAP_0805_FOOTPRINT,
    HEADER_1X03_THT_FOOTPRINT,
    INA4181_TSSOP20_FOOTPRINT,
    NET_TIE_2_FOOTPRINT,
    RES_0805_FOOTPRINT,
    RJ25_FOOTPRINT,
    SCREW_TERMINAL_2P_FOOTPRINT,
    SHUNT_R010_FOOTPRINT,
    SP3485_SOIC8_FOOTPRINT,
    XIAO_RP2040_DIP_FOOTPRINT,
    imp,
    power_flag,
)

from hardware.schematic import Schematic, UuidRegistry  # noqa: E402

HERE = Path(__file__).resolve().parent


def build() -> Schematic:
    reg = UuidRegistry(HERE / "uuids.json")
    sch = Schematic.new("legboard", reg, paper="A4")

    def stub_label(name: str, pin_xy: tuple[float, float], dx: float, dy: float = 0,
                   shape: str = "passive") -> None:
        end = (pin_xy[0] + dx, pin_xy[1] + dy)
        sch.wire(pin_xy, end)
        sch.label(name, end, shape=shape)

    imp(sch,
        "Seeed_Studio_XIAO_Series:XIAO-RP2040-DIP",
        "Hexapod_V2:SP3485CN",
        "Hexapod_V2:RJ25",
        "INA4181A3IPWR:INA4181A3IPWR",
        "Connector:Screw_Terminal_01x02",
        "Connector:Conn_01x03_Pin",
        "Device:NetTie_2",
        "Device:R_Small",
        "Device:C_Small",
        "Device:C_Polarized")

    # --- local controller ------------------------------------------------- #
    u1 = sch.place("Seeed_Studio_XIAO_Series:XIAO-RP2040-DIP", "U1", at=(70, 40),
                   value="XIAO-RP2040",
                   footprint=XIAO_RP2040_DIP_FOOTPRINT)
    sch.net("+5V", [u1.pin("14")])       # VBUS <- RJ11 logic power
    sch.net("+3.3V", [u1.pin("12")])     # 3V3 OUT (on-board regulator)
    sch.net("GND", [u1.pin("13")])
    sch.net("RP_TXD", [u1.pin("7")], rotation=180)     # P0/D6/TX -> RS485 DI
    sch.net("RP_RXD", [u1.pin("8")])     # P1/D7/RX <- RS485 RO
    sch.net("RS485_DE", [u1.pin("5")], rotation=180)   # P6/D4 -> driver enable
    sch.net("COXA_PWM", [u1.pin("11")])  # P3/D10
    sch.net("FEMUR_PWM", [u1.pin("10")])  # P4/D9
    sch.net("TIBIA_PWM", [u1.pin("9")])  # P2/D8
    sch.net("I_TOTAL_SENSE", [u1.pin("1")], rotation=180)    # P26/A0 (ADC)
    sch.net("I_COXA_SENSE", [u1.pin("2")], rotation=180)     # P27/A1 (ADC)
    sch.net("I_FEMUR_SENSE", [u1.pin("3")], rotation=180)    # P28/A2 (ADC)
    sch.net("I_TIBIA_SENSE", [u1.pin("4")], rotation=180)    # P29/A3 (ADC)

    # --- RS485 slave ------------------------------------------------------ #
    u2 = sch.place("Hexapod_V2:SP3485CN", "U2", at=(145, 72), value="SP3485CN",
                   footprint=SP3485_SOIC8_FOOTPRINT)
    sch.net("+3.3V", [u2.pin("8")])      # VCC
    sch.net("GND", [u2.pin("5")])
    sch.net("RP_RXD", [u2.pin("1")], rotation=180)     # RO
    sch.net("RP_TXD", [u2.pin("4")], rotation=180)     # DI
    sch.net("RS485_DE", [u2.pin("2"), u2.pin("3")], rotation=180)  # ~RE + DE tied
    sch.net("RS485_A", [u2.pin("6")])
    sch.net("RS485_B", [u2.pin("7")])

    # Bus termination 120R — DNP except on the two electrical ends of the bus.
    rt = sch.place("Device:R_Small", "R1", at=(170, 72), value="120 (DNP)",
                   rotation=90, footprint=RES_0805_FOOTPRINT)
    sch.net("RS485_A", [rt.pin("1")])
    sch.net("RS485_B", [rt.pin("2")])

    # --- RJ11 uplink: pins 1 and 6 intentionally unused ------------------ #
    j1 = sch.place("Hexapod_V2:RJ25", "J1", at=(25, 90), value="UPLINK_RJ11",
                   footprint=RJ25_FOOTPRINT)
    sch.net("GND", [j1.pin("2")])
    sch.net("RS485_A", [j1.pin("3")])
    sch.net("RS485_B", [j1.pin("4")])
    sch.net("+5V", [j1.pin("5")])

    # --- servo power input ------------------------------------------------ #
    j5 = sch.place("Connector:Screw_Terminal_01x02", "J5", at=(35, 140),
                   value="SERVO_PWR_IN", footprint=SCREW_TERMINAL_2P_FOOTPRINT)
    sch.net("+6V_IN", [j5.pin("1")])
    sch.net("GND", [j5.pin("2")])

    # --- servo connectors (signal / +6V / GND) --------------------------- #
    for ref, x, pwm, rail in (("J2", 120, "COXA_PWM", "+6V_COXA"),
                              ("J3", 155, "FEMUR_PWM", "+6V_FEMUR"),
                              ("J4", 190, "TIBIA_PWM", "+6V_TIBIA")):
        j = sch.place("Connector:Conn_01x03_Pin", ref, at=(x, 145), value=pwm[:-4],
                      footprint=HEADER_1X03_THT_FOOTPRINT)
        sch.net(pwm, [j.pin("1")])
        sch.net(rail, [j.pin("2")])
        sch.net("GND", [j.pin("3")])

    # --- current sensing -------------------------------------------------- #
    r_total = sch.place("Device:R_Small", "R2", at=(70, 140), value="0.01",
                        rotation=90, footprint=SHUNT_R010_FOOTPRINT)
    sch.net("+6V_IN", [r_total.pin("1")])
    sch.net("+6V", [r_total.pin("2")])

    for ref, x, branch in (("R3", 120, "+6V_COXA"),
                           ("R4", 155, "+6V_FEMUR"),
                           ("R5", 190, "+6V_TIBIA")):
        shunt = sch.place("Device:R_Small", ref, at=(x, 130), value="0.01",
                          rotation=90, footprint=SHUNT_R010_FOOTPRINT)
        sch.net("+6V", [shunt.pin("1")])
        sch.net(branch, [shunt.pin("2")])

    # Use dedicated thin sense nets between the shunts and INA inputs so PCB
    # routing rules can keep these traces narrow without changing the power-rail
    # width rules.
    nt1 = sch.place("Device:NetTie_2", "NT1", at=(82, 122),
                    footprint=NET_TIE_2_FOOTPRINT)
    sch.net("+6V_IN", [nt1.pin("1")])
    sch.net("+6V_IN_SNS", [nt1.pin("2")])

    nt2 = sch.place("Device:NetTie_2", "NT2", at=(92, 122),
                    footprint=NET_TIE_2_FOOTPRINT)
    sch.net("+6V", [nt2.pin("1")])
    sch.net("+6V_SNS", [nt2.pin("2")])

    for ref, x, rail, sense in (("NT3", 120, "+6V_COXA", "+6V_COXA_SNS"),
                                ("NT4", 155, "+6V_FEMUR", "+6V_FEMUR_SNS"),
                                ("NT5", 190, "+6V_TIBIA", "+6V_TIBIA_SNS")):
        nt = sch.place("Device:NetTie_2", ref, at=(x, 122),
                       footprint=NET_TIE_2_FOOTPRINT)
        sch.net(rail, [nt.pin("1")])
        sch.net(sense, [nt.pin("2")])

    u3 = sch.place("INA4181A3IPWR:INA4181A3IPWR", "U3", at=(225, 40),
                   value="INA4181A3IPWR", footprint=INA4181_TSSOP20_FOOTPRINT)
    stub_label("GND", u3.pin("1"), -15)
    stub_label("I_TOTAL_SENSE", u3.pin("2"), -15)
    stub_label("+6V_SNS", u3.pin("3"), -15)
    stub_label("+6V_IN_SNS", u3.pin("4"), -15)
    stub_label("+3.3V", u3.pin("5"), -15)
    stub_label("+6V_SNS", u3.pin("6"), -15)
    stub_label("+6V_COXA_SNS", u3.pin("7"), -15)
    stub_label("I_COXA_SENSE", u3.pin("8"), -15)
    stub_label("GND", u3.pin("9"), -15)
    stub_label("+6V_FEMUR_SNS", u3.pin("14"), 8)
    stub_label("+6V_SNS", u3.pin("15"), 8)
    stub_label("GND", u3.pin("16"), 8)
    stub_label("+6V_SNS", u3.pin("17"), 8)
    stub_label("+6V_TIBIA_SNS", u3.pin("18"), 8)
    stub_label("I_TIBIA_SENSE", u3.pin("19"), 8)
    stub_label("GND", u3.pin("20"), 8)
    stub_label("I_FEMUR_SENSE", u3.pin("13"), 8)

    # --- decoupling / bulk ------------------------------------------------ #
    c1 = sch.place("Device:C_Small", "C1", at=(145, 108), value="100nF",
                   footprint=CAP_0805_FOOTPRINT)
    sch.net("+3.3V", [c1.pin("1")])
    sch.net("GND", [c1.pin("2")])
    c2 = sch.place("Device:C_Polarized", "C2", at=(52, 140), value="470uF",
                   footprint=BULK_CAP_470UF_FOOTPRINT)
    sch.net("+6V", [c2.pin("1")])
    sch.net("GND", [c2.pin("2")])
    c3 = sch.place("Device:C_Small", "C3", at=(40, 40), value="100nF",
                   footprint=CAP_0805_FOOTPRINT)
    sch.net("+5V", [c3.pin("1")])
    sch.net("GND", [c3.pin("2")])
    c4 = sch.place("Device:C_Small", "C4", at=(248, 40), value="100nF",
                   footprint=CAP_0805_FOOTPRINT)
    sch.net("+3.3V", [c4.pin("1")])
    sch.net("GND", [c4.pin("2")])

    # --- ERC power flags -------------------------------------------------- #
    power_flag(sch, "#FLG1", (35, 175), "+5V")
    power_flag(sch, "#FLG2", (55, 175), "+3.3V")
    power_flag(sch, "#FLG3", (75, 175), "+6V")
    power_flag(sch, "#FLG4", (95, 175), "GND")
    power_flag(sch, "#FLG5", (115, 175), "+6V_IN")

    return sch


if __name__ == "__main__":
    build().write(HERE / "legboard.kicad_sch")
    print("wrote legboard.kicad_sch")
