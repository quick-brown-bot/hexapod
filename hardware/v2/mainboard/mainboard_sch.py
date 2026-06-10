"""Hexapod V2 — MainBoard schematic (source of truth).

Central logic + communication controller. Per the V2 architecture, this board
carries logic power, communication and control signals ONLY — never servo
current.

Contents:
* U1  ESP32-WROOM-32D — gait/IK/stabilisation/comms (the "intelligence").
* U2  SP3485CN RS485 transceiver — bus master to the six leg controllers.
* J_REG  3.3V regulator module header (5V in -> 3.3V out) — local logic rail.
* J_SBEC logic-power input from the MainPowerBoard SBEC (+5V).
* J_IMU  IMU breakout header (I2C + INT) — module abstraction, processed centrally.
* J1..J6 RJ11 (RJ25 6P) leg interfaces: GND / +5V logic / RS485 A / RS485 B.

Power model: SBEC +5V comes in, is distributed unchanged to the legs over RJ11
(logic power), and feeds a local 3V3 regulator module that powers the ESP32,
IMU and RS485 transceiver.

Run `python hardware/v2/mainboard/mainboard_sch.py` to (re)generate
`mainboard.kicad_sch`. Connectivity is by net label; placement is a loose grid
meant to be tidied in Eeschema.

NOTE: unused ESP32 GPIO pins are left open here — add no-connect flags in
Eeschema before relying on ERC.
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
    sch = Schematic.new("mainboard", reg, paper="A4")

    imp(sch,
        "SymbolsLib:ESP32-WROOM-32D",
        "Hexapod_V2:SP3485CN",
        "Hexapod_V2:RJ25",
        "Connector:Conn_01x02_Pin",
        "Connector:Conn_01x03_Pin",
        "Connector:Conn_01x05_Pin",
        "Device:R_Small",
        "Device:C_Small",
        "Device:C_Polarized",
        "Device:LED_Small")

    # --- main controller -------------------------------------------------- #
    u1 = sch.place("SymbolsLib:ESP32-WROOM-32D", "U1", at=(130, 95),
                   value="ESP32-WROOM-32D")
    sch.net("+3.3V", [u1.pin("V3.3")])
    sch.net("GND", [u1.pin("1")])
    sch.net("ESP_TXD", [u1.pin("IO17")])      # UART2 TX -> RS485 DI
    sch.net("ESP_RXD", [u1.pin("IO16")])      # UART2 RX <- RS485 RO
    sch.net("RS485_DE", [u1.pin("IO4")])      # driver enable / ~receiver enable
    sch.net("IMU_SDA", [u1.pin("IO21")])
    sch.net("IMU_SCL", [u1.pin("IO22")])
    sch.net("IMU_INT", [u1.pin("IO34")])

    # EN pull-up (boot).
    r_en = sch.place("Device:R_Small", "R2", at=(105, 80), value="10k", rotation=90)
    sch.net("+3.3V", [r_en.pin("1")])
    sch.net("ESP_EN", [r_en.pin("2"), u1.pin("EN")])

    # --- RS485 master ----------------------------------------------------- #
    u2 = sch.place("Hexapod_V2:SP3485CN", "U2", at=(195, 95), value="SP3485CN")
    sch.net("+3.3V", [u2.pin("8")])     # VCC
    sch.net("GND", [u2.pin("5")])       # GND
    sch.net("ESP_RXD", [u2.pin("1")])                  # RO
    sch.net("ESP_TXD", [u2.pin("4")])                  # DI
    sch.net("RS485_DE", [u2.pin("2"), u2.pin("3")])    # ~RE + DE tied
    sch.net("RS485_A", [u2.pin("6")])
    sch.net("RS485_B", [u2.pin("7")])

    # Bus termination 120R across A/B (fit on the master end of the bus).
    rt = sch.place("Device:R_Small", "R1", at=(220, 95), value="120", rotation=90)
    sch.net("RS485_A", [rt.pin("1")])
    sch.net("RS485_B", [rt.pin("2")])

    # --- power input & local 3V3 ----------------------------------------- #
    j_sbec = sch.place("Connector:Conn_01x02_Pin", "J7", at=(40, 90),
                       value="SBEC_5V_IN")
    sch.net("+5V", [j_sbec.pin("1")])
    sch.net("GND", [j_sbec.pin("2")])

    j_reg = sch.place("Connector:Conn_01x03_Pin", "J8", at=(70, 60),
                      value="3V3_REG_MODULE")
    sch.net("+5V", [j_reg.pin("1")])    # VIN
    sch.net("GND", [j_reg.pin("2")])
    sch.net("+3.3V", [j_reg.pin("3")])  # 3V3 OUT

    # --- IMU breakout (I2C) ---------------------------------------------- #
    j_imu = sch.place("Connector:Conn_01x05_Pin", "J9", at=(70, 120),
                      value="IMU_I2C")
    sch.net("+3.3V", [j_imu.pin("1")])
    sch.net("GND", [j_imu.pin("2")])
    sch.net("IMU_SDA", [j_imu.pin("3")])
    sch.net("IMU_SCL", [j_imu.pin("4")])
    sch.net("IMU_INT", [j_imu.pin("5")])

    # --- decoupling ------------------------------------------------------- #
    c1 = sch.place("Device:C_Polarized", "C1", at=(50, 110), value="10uF")
    sch.net("+5V", [c1.pin("1")])
    sch.net("GND", [c1.pin("2")])
    for ref, x, rail in (("C2", 150, "+3.3V"), ("C3", 175, "+3.3V"),
                         ("C4", 90, "+3.3V")):
        c = sch.place("Device:C_Small", ref, at=(x, 125), value="100nF")
        sch.net(rail, [c.pin("1")])
        sch.net("GND", [c.pin("2")])

    # --- power status LED ------------------------------------------------- #
    r_led = sch.place("Device:R_Small", "R3", at=(245, 55), value="1k", rotation=90)
    d_led = sch.place("Device:LED_Small", "D1", at=(245, 70), value="PWR",
                      rotation=90)
    sch.net("+3.3V", [r_led.pin("1")])
    sch.net("LED_PWR", [r_led.pin("2"), d_led.pin("2")])  # anode of LED_Small = pin 2
    sch.net("GND", [d_led.pin("1")])

    # --- RJ11 leg interfaces (x6) ---------------------------------------- #
    for i in range(1, 7):
        j = sch.place("Hexapod_V2:RJ25", f"J{i}", at=(40 + (i - 1) * 35, 175),
                      value=f"LEG{i}_RJ11")
        sch.net("GND", [j.pin("1")])
        sch.net("+5V", [j.pin("2")])
        sch.net("RS485_A", [j.pin("3")])
        sch.net("RS485_B", [j.pin("4")])

    # --- ERC power flags -------------------------------------------------- #
    power_flag(sch, "#FLG1", (40, 145), "+5V")
    power_flag(sch, "#FLG2", (60, 145), "+3.3V")
    power_flag(sch, "#FLG3", (80, 145), "GND")

    return sch


if __name__ == "__main__":
    build().write(HERE / "mainboard.kicad_sch")
    print("wrote mainboard.kicad_sch")
