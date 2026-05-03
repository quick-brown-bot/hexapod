# hex_controller_iface

## Role
Shared controller-facing interfaces consumed by controller core and controller input drivers.

## Public Headers
- controller.h
- controller_internal.h

## Design Intent
- Keep driver-facing controller contracts out of main component.
- Allow drivers to depend on a stable interface component instead of private main include paths.
- Preserve non-circular dependency direction for component graph.
