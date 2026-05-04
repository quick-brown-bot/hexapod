# hex_config_manager

## Role
Configuration domain management, registry dispatch, migration, and NVS persistence for robot/system settings.

## Responsibilities
- Own all namespace descriptors and parameter registries.
- Provide typed read/write APIs for system and joint configuration values.
- Load defaults, load persisted data, and run schema migrations during startup.
- Persist namespace data to NVS, including on-demand namespace save by name.

## Public Surface
- Header: config_manager.h
- Primary APIs:
  - config_manager_init
  - config_get_system
  - config_get_joint
  - config_set_system_* / config_set_joint_*
  - config_list_namespaces
  - config_list_parameters
  - config_get_parameter_info
  - config_manager_save_namespace
  - config_manager_save_namespace_by_name

## Internal Modules
- config_namespace_registry: runtime namespace descriptor registry.
- config_ns_system: system namespace descriptor and callbacks.
- config_ns_joint_calib: joint calibration namespace descriptor and callbacks.
- config_ns_leg_geometry: leg geometry namespace descriptor and callbacks.
- config_ns_motion_limits: motion limits namespace descriptor and callbacks.
- config_ns_controller: controller namespace descriptor and callbacks.
- config_ns_wifi: wifi namespace descriptor and callbacks.
- config_registry: parameter metadata registry.
- config_domain_controller_access: metadata and typed get/set helpers for controller namespace.
- config_domain_wifi_access: metadata and typed get/set helpers for wifi namespace.
- config_domain_defaults: default value seeding helpers.
- config_domain_persistence_nvs: namespace load/write hooks.
- config_migration: schema migration coordination.
- config_storage_nvs: low-level NVS read/write helpers.

## Integration
- Application startup initializes this component before controller and motion loops.
- RPC command handling queries and mutates values through this component APIs.
- Robot calibration/bootstrap code reads configuration through typed accessors.

## SDKConfig Requirements (Current Project)
- NVS support must be enabled in ESP-IDF (default project setting):
  - CONFIG_NVS_FLASH=y
- NVS partition must exist in partition table used by this project.
- This component assumes logging support is enabled:
  - CONFIG_LOG_DEFAULT_LEVEL (project-defined level)
