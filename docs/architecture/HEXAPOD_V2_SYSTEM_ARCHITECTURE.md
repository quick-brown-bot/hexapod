# Hexapod V2 System Architecture

## Status

Accepted architecture for Version 2.

This document defines the hardware architecture, power distribution,
communication topology, and subsystem responsibilities for the robot. The
schematic implementation of these boards lives at
[`hardware/v2/`](../../hardware/v2/README.md) (authored as code).

---

## Design Goals

Version 2 introduces distributed leg controllers and separation of logic from
power distribution.

Primary goals:

* simplify wiring,
* improve reliability,
* isolate high-current servo loads,
* improve diagnostics,
* support future expansion,
* reduce coupling between motion control and low-level actuation.

---

## System Overview

```text
                     LiPo Battery
                           |
            --------------------------------
            |                              |
        MainPowerBoard                 SBEC
            |                              |
      3x Servo UBECs                  MainBoard
            |                              |
            |                         ESP32 + IMU
            |                              |
            |                         RS485 Bus
            |                              |
            |      -----------------------------------------
            |      |       |       |       |       |       |
            |    Leg1    Leg2    Leg3    Leg4    Leg5    Leg6
            |   RP2040  RP2040  RP2040  RP2040  RP2040  RP2040
            |
      Servo Power Distribution
```

---

## Power Architecture

### Logic Power

Logic electronics are powered independently from servo power.

```text
LiPo -> SBEC -> MainBoard -> RJ11 -> Leg MCU
```

Logic power supplies: ESP32, IMU, RS485 transceivers, RP2040 controllers,
monitoring circuitry.

### Servo Power

Servo power is distributed separately from logic.

```text
LiPo -> 3x UBEC -> MainPowerBoard -> LegBoard -> Servos
```

Allocation:

```text
UBEC #1 -> Leg 1 + Leg 2
UBEC #2 -> Leg 3 + Leg 4
UBEC #3 -> Leg 5 + Leg 6
```

High-current servo loads never pass through MainBoard.

---

## Architectural Rule

**Servo current must never flow through MainBoard.** MainBoard carries logic
power, communication, and control signals only. All high-current distribution
belongs to MainPowerBoard and LegBoards.

---

## MainPowerBoard

Responsible for all high-current power distribution: battery input; servo power
distribution from the three UBECs to the six legs; bulk capacitance for
transient servo loads (1000-2200 µF low-ESR per UBEC, validated experimentally);
power monitoring (battery, UBEC outputs, optional UBEC current); visual status
indicators (UBEC #1/#2/#3 present, SBEC present); and protection (main fuse,
reverse-polarity protection, master power switch).

---

## MainBoard

The central logic and communication controller.

* **Main controller** — hosts the ESP32: controller input, gait/trajectory
  generation, inverse kinematics, body stabilisation, IMU processing,
  communication management, diagnostics.
* **IMU** — hosted and processed centrally; no IMU processing on leg controllers.
* **RS485 master** — SP3485 transceiver, bus control, termination if required.
* **RJ11 leg interfaces** — one per leg.
* **Logic power distribution** — SBEC power to ESP32, IMU, RS485, and all legs.
* **Monitoring** — ingests telemetry from MainPowerBoard and LegBoards.
* **Decoupling** — local filtering for ESP32, IMU, comms.

RJ11 pinout:

| Pin | Function    |
| --- | ----------- |
| 1   | GND         |
| 2   | Logic Power |
| 3   | RS485 A     |
| 4   | RS485 B     |

RJ11 carries communication and logic power only.

---

## LegBoard

An intelligent local actuator controller; each leg has its own independent
electronics: XIAO RP2040, SP3485 RS485 transceiver, servo connectors, current
sensing, voltage monitoring, and local filtering capacitors.

Responsibilities: receive commands over RS485 and report telemetry; generate
servo PWM locally (MainBoard never generates servo PWM); local diagnostics
(per-servo current, total leg current, leg supply voltage, comms health);
watchdog on communication timeout; and local trajectory interpolation between
received motion targets for smooth motion at lower command rates.

---

## Motion Architecture

Motion planning remains centralized. The ESP32 owns gait generation, body
control, inverse kinematics, and stabilisation. Leg controllers do not execute
inverse kinematics.

Command model — the ESP32 sends a target plus an expected arrival time:

```text
coxa_target
femur_target
tibia_target
arrival_time
```

The RP2040 interpolates intermediate points locally: smoother motion, less
visible stepping, lower bandwidth, and tolerance to short communication delays.

---

## IMU Integration

IMU processing remains entirely centralized — robot orientation affects all legs
simultaneously, stabilisation requires full-body awareness, and a single motion
authority simplifies the architecture. The ESP32 continuously updates
trajectories from IMU feedback; leg controllers remain unaware of body
orientation.

---

## Control Frequencies

* **MainBoard** — ~100 Hz: IMU update, gait generation, inverse kinematics,
  trajectory generation, communication.
* **LegBoard** — ~500-1000 Hz internal loop: interpolation, watchdogs,
  diagnostics, telemetry sampling. PWM generation is hardware-timed.

---

## Core Architectural Principle

MainBoard owns intelligence. LegBoards own execution. MainPowerBoard owns energy
distribution. This separation lets each subsystem evolve independently while
keeping robot-level motion control centralized.
