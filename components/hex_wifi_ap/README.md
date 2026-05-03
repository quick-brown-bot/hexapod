# hex_wifi_ap

## Role
WiFi Access Point bootstrap utility for local robot connectivity.

## Responsibilities
- Initialize WiFi AP mode and network stack once.
- Build AP SSID using fixed, MAC-suffix, or random-suffix strategies.
- Apply AP options such as password, channel, and max clients.
- Expose active SSID for diagnostics and user feedback.

## Public Surface
- Header: wifi_ap.h
- Core APIs:
  - wifi_ap_init_with_options
  - wifi_ap_init_once
  - wifi_ap_get_ssid

## Integration
- Used by the WiFi TCP controller driver to provide an AP for command/control sessions.
- Keeps AP bring-up logic outside controller and RPC core modules.
