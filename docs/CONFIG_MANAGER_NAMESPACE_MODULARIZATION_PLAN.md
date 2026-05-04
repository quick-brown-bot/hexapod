# Config Manager Namespace Modularization Plan

## Status

- Branch created: `refactor/config-manager-namespace-plan`
- This document now tracks both plan and implementation progress.
- Phases 1-4: completed and build-validated.
- Phase 5: in progress.

## Implementation Progress (Current)

Completed:
- Phase 1: registration/wiring extracted to namespace catalog.
- Phase 2: namespace state ownership moved out of `config_manager.c` into namespace modules.
- Phase 3: defaults and persistence split by namespace; multi-namespace persistence hub removed.
- Phase 4: header/API modularization completed.
  - Namespace-specific types/APIs moved to namespace headers.
  - Core API split into:
    - `config_manager_core_types.h`
    - `config_manager_runtime_api.h`
    - `config_manager_param_api.h`
  - Call sites in consuming components updated.
  - Compatibility aggregate header removed.

Remaining for Phase 5:
- Move each namespace into its own folder under `components/hex_config_manager/namespaces/*`.
- Enforce one-namespace-per-functional-file rule by directory structure, not only naming.
- Validate dummy namespace add flow requires only:
  - new namespace folder/files
  - single catalog registration edit
  - optional tests/docs
- Run runtime validation checklist (`list`, `get`, `set`, `setpersist`, `save`, reboot persistence).
- Update checklist instruction file:
  - `c:\code\hexapod\.github\instructions\config-manager-namespace-checklist.instructions.md`

## Goals

Refactor `hex_config_manager` so adding a new namespace is easy, predictable, and localized.

Target outcomes:
- Each namespace owns its own files and implementation logic.
- Adding a namespace should require changes in one registration catalog file plus the new namespace files.
- No functional file should contain functions from multiple namespaces.
- Every namespace should implement the same common interface contract.

## Decisions (Locked)

1. Prioritize readability and ease of understanding over minimizing file count.
2. CMake globbing is acceptable if it works reliably with tooling and CI.
3. No compatibility shims. Refactor is direct; old structures/APIs are removed when replaced.

## Current Pain Points

Primary growth hotspots today:
- `components/hex_config_manager/config_manager.c`
- `components/hex_config_manager/config_domain_persistence_nvs.c`
- `components/hex_config_manager/config_manager_runtime_api.h`
- `components/hex_config_manager/config_manager_param_api.h`
- `components/hex_config_manager/config_manager_core_types.h`
- `components/hex_config_manager/config_domain_defaults.c`

Reasons these grow each namespace:
- Per-namespace cache/context globals are centralized in one file.
- Per-namespace defaults and NVS key mappings are centralized in shared files.
- Namespace enum/types/API are all in one header.

## Target Architecture

### 1. Namespace Package Layout

Each namespace is self-contained under a dedicated folder.

Example structure:

- `components/hex_config_manager/namespaces/system/`
  - `system_module.c`
  - `system_module.h`
  - `system_types.h`
  - `system_defaults.c`
  - `system_persistence_nvs.c`
  - `system_access.c`
- Repeat same pattern for `joint_cal`, `leg_geom`, `motion_lim`, `controller`, `wifi`.

Rules:
- Namespace files only implement one namespace.
- No file under `namespaces/*` references parameters or key formats of another namespace.

### 2. Common Namespace Interface

All namespaces implement one shared contract (descriptor-driven):
- `load_defaults`
- `load_from_nvs`
- `write_defaults_to_nvs`
- `save`
- `list_parameters`
- `get_parameter_info`
- typed get/set callbacks (`bool`, `int32`, `uint32`, `float`, `string`) where supported

Each namespace module exports:
- `const config_namespace_descriptor_t* <ns>_descriptor(void)`
- `void* <ns>_context(void)` (or equivalent context provider)

### 3. Single Registration Catalog

Introduce one catalog file that is the only cross-namespace edit point:
- `components/hex_config_manager/config_namespace_catalog.c`
- `components/hex_config_manager/config_namespace_catalog.h`

Catalog responsibilities:
- List namespace modules.
- Register descriptors/contexts in deterministic order.

`config_manager.c` should consume catalog output and not declare per-namespace cache/context variables.

### 4. Header Boundary Cleanup

Split the core API surface into:
- `config_manager_core_types.h` for shared enums/state/metadata structs.
- `config_manager_runtime_api.h` for lifecycle and persistence operations.
- `config_manager_param_api.h` for typed get/set and parameter discovery APIs.
- Namespace-specific types moved to namespace headers.

No monolithic namespace type hub in manager-facing headers.

### 5. Build System

Use CMake globbing for namespace source collection:
- `file(GLOB CONFIGURE_DEPENDS ...)`

Fallback policy:
- If globbing causes tooling or CI instability, switch to one explicit namespace source list file (single catalog list), not scattered edits.

## Refactor Phases

### Phase 1: Registration and Wiring Foundation

Deliverables:
- Add namespace catalog files.
- Move namespace registration table out of `config_manager.c` into catalog.
- Keep behavior unchanged.

Expected edits:
- `config_manager.c`
- `config_namespace_catalog.c/h`
- `CMakeLists.txt`

### Phase 2: Move Namespace State Ownership

Deliverables:
- Move per-namespace cache/context storage from `config_manager.c` into namespace modules.
- `config_manager.c` no longer declares namespace-specific globals.

Expected edits:
- namespace modules
- `config_manager.c`
- catalog

### Phase 3: Split Defaults and Persistence by Namespace

Deliverables:
- Remove multi-namespace defaults file usage for namespace logic.
- Remove multi-namespace persistence file usage for namespace logic.
- Introduce shared NVS utility helpers only for generic primitives.

Expected edits:
- create per-namespace defaults/persistence files
- reduce/remove `config_domain_defaults.c` and `config_domain_persistence_nvs.c` as namespace hubs

### Phase 4: Header/API Modularization (No Shims)

Deliverables:
- Move namespace-specific types and bulk APIs into namespace headers.
- Keep only generic manager and typed/discovery APIs in core header.
- Remove replaced declarations from core header directly.

Expected edits:
- `config_manager_core_types.h`
- `config_manager_runtime_api.h`
- `config_manager_param_api.h`
- namespace headers
- call sites in consuming components

### Phase 5: Final Cleanup and Validation

Deliverables:
- Enforce one-namespace-per-functional-file rule.
- Enforce one-namespace-per-folder layout under `namespaces/*`.
- Validate adding a dummy namespace only touches:
  - new namespace folder files
  - single registration catalog file
  - optional tests/docs

## Acceptance Criteria

Architecture is accepted when all are true:
- Adding a namespace does not require editing `config_manager.c` functional logic.
- Adding a namespace does not require editing shared defaults or shared persistence hubs.
- Each namespace has a consistent module shape and callback contract.
- No functional implementation file mixes logic from multiple namespaces.
- Build still succeeds and runtime namespace list/save/load flows are unchanged behaviorally.

## Risks and Mitigations

1. Risk: Large rename/move churn can break include paths.
- Mitigation: phase changes with build verification after each phase.

2. Risk: Globbing may behave unexpectedly in some tooling.
- Mitigation: use `CONFIGURE_DEPENDS`; if unstable, replace with single explicit source manifest file.

3. Risk: Runtime regressions in load/save defaults order.
- Mitigation: keep descriptor order explicit in catalog and verify with `list namespaces` plus namespace save/load checks.

## Verification Checklist Per Phase

- Build succeeds (`esp-idf build`).
- `config_manager_init` succeeds.
- `list namespaces` includes all existing namespaces.
- `list <namespace>`, `get`, `set`, `setpersist`, and `save <namespace>` still work.
- Reboot persistence remains correct for touched namespaces.

## Out of Scope (This Plan)

- New feature parameters.
- Schema version bump policy redesign.
- RPC protocol redesign.

## Finalization Follow-Up

After completing the refactor, update the namespace implementation checklist instruction file to reflect the new architecture and lessons learned:

- File to update:
  - `c:\code\hexapod\.github\instructions\config-manager-namespace-checklist.instructions.md`
- Required updates:
  - Replace old steps that assume edits in shared multi-namespace files.
  - Document the new single registration-catalog flow.
  - Document the per-namespace file/folder contract and common interface expectations.
  - Add validation checks that enforce no cross-namespace functional files and no shim-based migrations.
  - Refresh examples and terminology so future namespace additions follow the modularized architecture by default.
