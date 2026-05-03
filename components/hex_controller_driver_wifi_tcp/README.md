# hex_controller_driver_wifi_tcp

## Role
WiFi TCP controller and RPC transport driver.

## Responsibilities
- Start AP-backed TCP server for remote command sessions.
- Accept client connections and stream incoming lines into RPC transport RX queue.
- Register transport sender for outbound RPC responses over active TCP socket.
- Enforce idle timeout and trigger controller failsafe when disconnected.

## Public Surface
- Header: controller_wifi_tcp.h
- Core APIs:
  - controller_driver_init_wifi_tcp
  - controller_wifi_tcp_send_raw

## Integration
- Uses hex_wifi_ap for AP bring-up.
- Uses hex_rpc_transport for inbound/outbound RPC data flow.
- Uses controller_internal helpers for connection and failsafe state.
