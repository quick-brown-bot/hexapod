# hex_controller_driver_flysky_ibus

## Role
FlySky iBUS controller input driver.

## Responsibilities
- Configure UART for iBUS reception.
- Parse iBUS frames and normalize channels to shared controller format.
- Forward normalized channel updates as internal RPC controller commands.
- Detect connection timeout and trigger controller failsafe.

## Public Surface
- Header: controller_flysky_ibus.h
- Core API:
  - controller_driver_init_flysky_ibus

## Integration
- Registered by controller core through driver selection.
- Uses controller_internal helpers for connection state and failsafe behavior.
- Uses hex_rpc_transport to inject internal command messages.
