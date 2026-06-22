# Hexapod Documentation

Documentation is organized by version and shared scope.

## Structure

```
docs/
  common/          — applies to both V1 and V2 (ESP32 input drivers, RPC, WiFi, config platform)
  v1/              — V1-specific: direct-PWM ESP32-only firmware
  v2/              — V2-specific: RS485 distributed architecture with RP2040 leg controllers
  plans/           — project-wide TODO and forward-looking plans
```

---

## Common

These docs describe subsystems that are shared across firmware versions: controller
input drivers, the RPC system, WiFi and Bluetooth transports, and the configuration
platform.

### Configuration

- [common/configuration/CONFIGURATION_PERSISTENCE_DESIGN.md](common/configuration/CONFIGURATION_PERSISTENCE_DESIGN.md)
- [common/configuration/CONFIG_MANAGER_NAMESPACE_TEMPLATE.md](common/configuration/CONFIG_MANAGER_NAMESPACE_TEMPLATE.md)

### Interfaces

- [common/interfaces/RPC_USER_GUIDE.md](common/interfaces/RPC_USER_GUIDE.md)
- [common/interfaces/RPC_SYSTEM_DESIGN.md](common/interfaces/RPC_SYSTEM_DESIGN.md)
- [common/interfaces/CONTROLLER_DRIVERS.md](common/interfaces/CONTROLLER_DRIVERS.md)
- [common/interfaces/WIFI_TCP_PROTOCOL.md](common/interfaces/WIFI_TCP_PROTOCOL.md)
- [common/interfaces/WIFI_NETWORK_MODES.md](common/interfaces/WIFI_NETWORK_MODES.md)
- [common/interfaces/BLUETOOTH_CLASSIC_PROTOCOL.md](common/interfaces/BLUETOOTH_CLASSIC_PROTOCOL.md)

---

## V1

Direct-PWM ESP32-only firmware. Hardware: [`hardware/v1/`](../hardware/v1/README.md).
Firmware: [`firmware/v1/`](../firmware/v1/mainboard/README.md).

- [v1/architecture/SYSTEM_ARCHITECTURE.md](v1/architecture/SYSTEM_ARCHITECTURE.md)
- [v1/architecture/HARDWARE_AND_MECHANICS.md](v1/architecture/HARDWARE_AND_MECHANICS.md)
- [v1/development/README.md](v1/development/README.md)
- [v1/plans/KPP_IMPLEMENTATION_PLAN.md](v1/plans/KPP_IMPLEMENTATION_PLAN.md)

---

## V2

RS485 distributed architecture: ESP32 mainboard + RP2040 leg controllers.
Hardware: [`hardware/v2/`](../hardware/v2/README.md).
Firmware: [`firmware/v2/`](../firmware/v2/leg/).

- [v2/architecture/SYSTEM_ARCHITECTURE.md](v2/architecture/SYSTEM_ARCHITECTURE.md)
- [v2/architecture/HARDWARE_AND_MECHANICS.md](v2/architecture/HARDWARE_AND_MECHANICS.md)
- [v2/interfaces/RS485_PROTOCOL.md](v2/interfaces/RS485_PROTOCOL.md)
- [v2/development/README.md](v2/development/README.md)

---

## Plans

- [plans/TODO.md](plans/TODO.md) — project-wide feature backlog and research items
