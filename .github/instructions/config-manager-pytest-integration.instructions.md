---
name: "Config Manager Pytest Integration Tests"
description: "Use when creating or refactoring pytest integration tests for hex_config_manager namespaces. Enforces network connection flow (reuse current connection or connect to first HEXAPOD_xxx SSID), command validation via live transport, and namespace-first test organization."
applyTo: "test/**/*.py, **/pytest_*.py"
---
# Config Manager Pytest Integration Tests

Use this guidance when writing Python integration tests that exercise `hex_config_manager` behavior through live robot command channels.

## Scope

- Prefer pytest integration tests over ad-hoc scripts for namespace validation.
- Test through externally visible commands (RPC/CLI/TCP) rather than calling firmware internals directly.
- Keep each namespace covered by at least one dedicated test module.

## Network Connection Contract

- Before issuing config commands, first check if the host is already connected to a matching robot AP (`HEXAPOD_xxx`).
- If already connected, reuse the current connection and continue.
- If not connected, discover available SSIDs and connect to the first `HEXAPOD_xxx` network.
- If no matching network is found, fail clearly with a skip/fail message that explains discovery results.
- Keep connection/bootstrap logic centralized in fixtures or helpers (for example `conftest.py`) so test bodies remain focused on behavior.

## Test Organization

- Split tests by namespace first (for example `test_config_ns_system.py`, `test_config_ns_leg_geom.py`, `test_config_ns_motion_lim.py`, `test_config_ns_servo_map.py`).
- Within each namespace module, split tests into logical behavior areas.
- Prefer explicit area groupings such as:
  - listing/discovery (`list namespaces`, `list <ns>`)
  - read semantics (`get`, type/format assertions)
  - write semantics (`set`, validation/error handling)
  - persistence (`setpersist`/`save`, reboot/reconnect, readback)
  - bounds/constraints (min/max, enum/domain validation)
  - integration impact (runtime behavior or downstream consumer changes)

## Assertions and Reliability

- Assert both transport-level success and semantic correctness of returned values.
- Use deterministic setup/teardown so tests can run repeatedly on a real device.
- Avoid order-coupled tests; each test should set up its own required state or use scoped fixtures.
- Reset or restore mutated configuration values when practical to avoid cross-test contamination.

## Parameterization and Coverage

- Use `pytest.mark.parametrize` for repeated checks across parameters in the same namespace.
- Cover at least one happy-path, one invalid input path, and one persistence path per namespace area.
- Prefer concise helper functions for repetitive command send/parse logic.

## Logging and Diagnostics

- Include clear assertion messages that name namespace and parameter under test.
- Log connection choice (reused current network vs discovered SSID) to simplify triage.
- On command failure, surface command text and parsed response in the failure output.

## Anti-patterns

- Do not hardcode a single SSID value when `HEXAPOD_xxx` discovery is required.
- Do not mix multiple namespaces heavily in one test module unless intentionally validating cross-namespace behavior.
- Do not rely on implicit device state left by previous tests.