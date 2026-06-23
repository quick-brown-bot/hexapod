# Hardware And Mechanics

## Purpose

This document collects the physical robot context that supports the firmware architecture: control hardware, actuator layout, power topology, and the mechanical model assumed by the motion stack.

## Hardware Summary

- Controller MCU: ESP32 using ESP-IDF and FreeRTOS.
- Actuators: 18 hobby servos arranged as 3 joints per leg across 6 legs.
- Power source: 4S LiPo battery.
- Locomotion power distribution: separate UBEC paths are used to distribute servo load.
- Logic power: ESP32 and auxiliary electronics are isolated on a separate regulator path.
- Signal generation: mixed MCPWM and LEDC allocation is used to produce the full set of servo control signals.

## Mechanical Layout

Each leg has three joints:

1. Coxa for yaw and horizontal reach.
2. Femur for the main pitch lift motion.
3. Tibia for distal extension and stance height control.

The robot uses a custom 3D-printed frame and leg structure sized around standardized servo dimensions and the target stance geometry.

## Nominal Link Geometry

- Coxa length: `0.068 m`
- Femur length: `0.088 m`
- Tibia length: `0.127 m`

These dimensions define the nominal leg model used by inverse kinematics and related runtime configuration.

## Coordinate Conventions

- Body frame: `+X` forward, `+Y` left, `+Z` up.
- Leg-local planning frame: leg mount transforms rotate body-frame targets into each leg's local coordinate system before IK is solved.

The exact per-leg mount poses are part of the robot configuration domain because they are consumed by the motion stack rather than treated as hardcoded documentation-only values.

## Why This Matters For Firmware

- Actuation design is constrained by the ESP32 PWM peripheral budget, which is why the firmware mixes MCPWM and LEDC.
- Runtime geometry and servo mapping are configuration-backed so that the motion stack can rely on a single source of truth.
- Power isolation is part of system reliability, especially when many high-torque servos move simultaneously.

## Related Docs

- [SYSTEM_ARCHITECTURE.md](SYSTEM_ARCHITECTURE.md)
- [../configuration/CONFIGURATION_PERSISTENCE_DESIGN.md](../configuration/CONFIGURATION_PERSISTENCE_DESIGN.md)