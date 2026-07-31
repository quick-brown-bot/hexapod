# Hexapod Documentation

## Structure

```
docs/
  common/          — controller input drivers, RPC, WiFi, config platform
  architecture/    — system and hardware architecture
  development/     — development setup and workflow
  plans/           — project-wide TODO and forward-looking plans
```

---

## Common

These docs describe subsystems that are shared across firmware: controller
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

## Architecture & Development

Direct-PWM ESP32-only firmware. Hardware: [`hardware/`](../hardware/README.md).
Firmware: [`firmware/mainboard/`](../firmware/mainboard/README.md).

- [architecture/SYSTEM_ARCHITECTURE.md](architecture/SYSTEM_ARCHITECTURE.md)
- [architecture/HARDWARE_AND_MECHANICS.md](architecture/HARDWARE_AND_MECHANICS.md)
- [development/README.md](development/README.md)

---

## Plans

- [plans/KPP_IMPLEMENTATION_PLAN.md](plans/KPP_IMPLEMENTATION_PLAN.md)
