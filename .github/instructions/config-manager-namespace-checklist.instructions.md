---
name: "Config Manager Namespace Checklist"
description: "Use when adding or refactoring a hex_config_manager namespace (for example leg_geom, motion_lim, servo_map). Enforces a complete checklist across enum wiring, descriptor module, defaults, NVS persistence, parameter discovery, build registration, and runtime validation."
applyTo: "components/hex_config_manager/**"
---
# Config Manager Namespace Checklist

When creating a new namespace in `hex_config_manager`, follow this checklist in order.

## Naming Rules

- Use short, console-friendly namespace string names (for example `system`, `joint_cal`, `leg_geom`, `motion_lim`).
- Keep namespace string names stable and consistent across descriptor, RPC, and docs.
- Prefer concise parameter names that are easy to type in console commands.

## 1) Namespace ID and Core Types

- Add a new enum entry in `config_namespace_t` in `config_manager_core_types.h` before `CONFIG_NS_COUNT`.
- Add or update namespace-specific config struct type(s) in namespace API headers (not manager core headers).
- Keep manager-facing API generic through `config_manager_runtime_api.h` and `config_manager_param_api.h`.

## 2) Namespace Folder Contract

- Create one dedicated namespace folder under `components/hex_config_manager/namespaces/<namespace>/`.
- Keep all namespace-owned implementation files in that folder.
- Required minimum files:
  - `config_ns_<namespace>.h`
  - `config_ns_<namespace>.c`
  - `config_ns_<namespace>_api.h` (if namespace exposes public types/accessors)
  - namespace defaults/persistence/access files as needed
- Do not place namespace functional logic in root-level manager files.

## 3) Descriptor Module (One Namespace Per Functional File)

- Create a dedicated descriptor header/source pair:
  - `namespaces/<namespace>/config_ns_<namespace>.h`
  - `namespaces/<namespace>/config_ns_<namespace>.c`
- Add a namespace context struct in the header with:
  - `nvs_handle_t*`
  - `namespace_dirty` pointer
  - `namespace_loaded` pointer
  - pointer to namespace cache struct
- Export `const config_namespace_descriptor_t g_<namespace>_namespace_descriptor`.
- In source, implement descriptor callbacks for:
  - `load_defaults`
  - `load_from_nvs`
  - `write_defaults_to_nvs`
  - `save`
  - typed get/set handlers relevant to the namespace
  - parameter listing and parameter info handlers

## 4) Register Descriptor in Namespace Catalog

- Add include for the new descriptor header in `config_namespace_catalog.c`.
- Append descriptor/context entry in catalog registration order.
- Ensure registration continues through `config_namespace_registry_register(descriptor, context)`.
- Do not add namespace globals or descriptor wiring to `config_manager.c`.

## 5) Defaults Layer

- Implement namespace defaults in namespace-local files inside `namespaces/<namespace>/`.
- Keep defaults aligned with existing runtime behavior used elsewhere in the project.

## 6) NVS Persistence Layer

- Implement write defaults/load/save in namespace-local persistence file(s) under `namespaces/<namespace>/`.
- Use compact key formats and keep key naming consistent.
- Commit writes with `nvs_commit` in save/default initialization paths.

## 7) Parameter Discovery and Metadata

- If namespace is generic/typed, implement:
  - parameter list generation
  - parameter parsing
  - metadata/constraint reporting
- Ensure `config_list_parameters` and `config_get_parameter_info` work for the namespace.
- Enforce validation ranges in set handlers.

## 8) Build Wiring

- Add new namespace source file(s) in `components/hex_config_manager/CMakeLists.txt` under the matching `namespaces/<namespace>/` path.
- Ensure new namespace folder is available through component include directories when public headers are consumed externally.
- Verify the new module is compiled in build output.

## 9) Runtime Listing and Save Path Validation

- Confirm `list namespaces` includes new namespace.
- Confirm `save <namespace>` works via `config_manager_save_namespace_by_name`.
- Ensure `config_list_namespaces` indexes output by `ns_id`, not registry iteration index.
- Verify every affected runtime consumer actually reads values from the new namespace at runtime (not just that the namespace exists).
- If a subsystem still uses compile-time constants or local defaults, wire it to the config manager namespace before considering the namespace complete.
- If namespace still does not appear at runtime, verify flashed firmware is current (not stale binary).

## 9.1) Runtime Configuration Contract (Fail Fast)

- Do not assume hardcoded defaults are acceptable when configuration is expected to come from NVS/config manager.
- For critical runtime subsystems, fail fast with clear logs if required namespace data is missing, unloaded, or invalid.
- Validate initialization order so config manager loads before subsystems that consume persisted configuration.
- Log the configuration source at startup (for example namespace-backed vs fallback) and prefer namespace-backed only for completed integrations.

## 10) Documentation Sync

- Update design docs and examples to the short namespace string name.
- Keep RPC examples aligned with real namespace strings and parameter names.
- Keep folder-path examples aligned with `namespaces/<namespace>/` structure.

## 11) Verification Checklist

- Build succeeds with no new errors.
- `list namespaces` includes the new namespace.
- `list <namespace>` returns expected parameter names.
- `get` and `set` work for at least one parameter.
- `setpersist` and `save <namespace>` persist and survive reboot.
- Runtime subsystem behavior changes when namespace values change (proves real consumption path).
- Startup fails clearly (or blocks subsystem start) when required namespace config cannot be loaded for strict integrations.
- Adding a namespace only required:
  - new files in one namespace folder
  - one registration edit in `config_namespace_catalog.c`
  - optional docs/tests updates
