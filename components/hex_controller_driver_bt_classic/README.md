# hex_controller_driver_bt_classic

## Role
Bluetooth Classic SPP controller and RPC transport driver.

## Responsibilities
- Initialize Bluetooth Classic stack and SPP server.
- Handle pairing/authentication callbacks and connection lifecycle.
- Forward incoming SPP payloads into RPC transport RX queue.
- Register transport sender for outbound RPC responses over SPP.
- Trigger failsafe on disconnect or timeout conditions.

## Public Surface
- Header: controller_bt_classic.h
- Core APIs:
  - controller_driver_init_bt_classic
  - controller_bt_classic_send_raw
  - controller_bt_get_device_name

## Integration
- Registered by controller core through driver selection.
- Uses hex_rpc_transport for RPC message exchange.
- Uses controller_internal helpers for shared connection and failsafe behavior.
