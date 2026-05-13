# Development Setup

## Purpose

This document is the software-side starting point for contributors. It covers the expected development environment, editor workflow, firmware build loop, and host-side test setup.

## Development Environment

### Primary Toolchain

- ESP-IDF 5.x is the expected firmware framework and build environment.
- Visual Studio Code is the intended editor workflow for this project.
- The official ESP-IDF extension for VS Code should be installed and configured.

Recommended baseline:

1. Install ESP-IDF 5.x using the official Espressif tooling for your platform.
2. Install Visual Studio Code.
3. Install the official ESP-IDF extension in VS Code.
4. Use the extension to configure the ESP-IDF tools, Python environment, and target toolchain.

## Repository Orientation

- `main/` contains top-level bootstrapping and orchestration.
- `components/` contains the modular firmware subsystems.
- `docs/` contains architecture, development, configuration, interface, plan, and archive documentation.
- `test/` contains host-side integration tests.

If you are new to the codebase, read these first:

1. [../architecture/SYSTEM_ARCHITECTURE.md](../architecture/SYSTEM_ARCHITECTURE.md)
2. [../architecture/HARDWARE_AND_MECHANICS.md](../architecture/HARDWARE_AND_MECHANICS.md)
3. [../configuration/CONFIGURATION_PERSISTENCE_DESIGN.md](../configuration/CONFIGURATION_PERSISTENCE_DESIGN.md)

## VS Code Workflow

This project is designed around the VS Code plus ESP-IDF extension workflow.

Typical setup tasks in VS Code:

1. Open the repository root as the workspace.
2. Let the ESP-IDF extension select or create the correct ESP-IDF environment.
3. Configure the target as ESP32 if the extension asks for it.
4. Use the extension commands or integrated terminal for build, flash, and monitor cycles.

The editor workflow matters because it keeps ESP-IDF tools, Python, CMake generation, and serial flashing aligned with the project layout.

## Build, Flash, And Monitor

From an ESP-IDF-enabled terminal, the standard loop is:

```bash
idf.py build
idf.py flash
idf.py monitor
```

Notes:

- Run commands from the repository root.
- The generated build output is expected under `build/`.
- If you change target configuration or environment state, regenerate through the ESP-IDF workflow rather than trying to hand-edit generated files.

## Python And Host-Side Tests

The repository includes host-side pytest integration tests for runtime command and configuration flows.

Install test dependencies:

```bash
pip install -r test/requirements.txt
```

Run a focused test file:

```bash
python -m pytest -q -p no:embedded test/test_config_general_listing.py
```

Why `-p no:embedded` is used here:

- Some environments auto-load `pytest-embedded`, which is not needed for these host-side RPC/config tests.
- Disabling that plugin keeps the test run aligned with the intended Windows host workflow.

## Test Environment Assumptions

- The current host-side tests are intended for a Windows development machine.
- Tests expect access to a reachable robot endpoint over Wi-Fi.
- Some flows reuse the current `HEXABOT_*` or `HEXAPOD_*` network, depending on the test utility behavior in the environment.

If needed, override the target endpoint with environment variables before running tests:

```bash
set HEXABOT_IP=<robot_ip>
set HEXABOT_PORT=<robot_port>
```

## Practical First Steps

For a new software contributor, the shortest sensible ramp is:

1. Install and validate ESP-IDF in VS Code.
2. Build the firmware once successfully.
3. Read the system architecture doc to understand the component split.
4. Run one focused host-side test file.
5. Only then start changing component code or configuration flows.

## Related Docs

- [../README.md](../README.md)
- [../architecture/SYSTEM_ARCHITECTURE.md](../architecture/SYSTEM_ARCHITECTURE.md)
- [../interfaces/RPC_USER_GUIDE.md](../interfaces/RPC_USER_GUIDE.md)
- [../configuration/CONFIGURATION_PERSISTENCE_DESIGN.md](../configuration/CONFIGURATION_PERSISTENCE_DESIGN.md)