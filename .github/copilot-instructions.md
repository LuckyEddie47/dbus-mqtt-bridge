# Copilot Instructions for dbus-mqtt-bridge

## Project Overview
- **Purpose:** Bi-directional bridge between DBus (Linux IPC) and MQTT (IoT messaging), implemented as a high-performance Linux system service.
- **Core Components:**
  - `src/`: Main logic (see `Bridge.cpp`, `DbusManager.cpp`, `MqttManager.cpp`, `Config.cpp`)
  - `include/`: Public headers for each component
  - `simulators/`: Standalone DBus and MQTT simulators for testing
  - `tests/`: Unit tests using GoogleTest
  - `build/`: Build artifacts, config examples, and test binaries

## Architecture & Data Flow
- **Bridge:** Central orchestrator, connects DBus and MQTT managers, handles message translation and routing.
- **DbusManager:** Listens for DBus signals/methods, serializes arguments to JSON, and forwards to MQTT.
- **MqttManager:** Subscribes/publishes to MQTT topics, deserializes JSON payloads, and invokes DBus methods/signals.
- **Config:** Loads YAML config (see `build/config.yaml`), defines mappings and runtime options.
- **Data Format:** All cross-bus payloads are JSON arrays matching DBus signatures.

## Developer Workflows
- **Build:**
  - `mkdir build && cd build && cmake .. && make -j$(nproc)`
- **Test:**
  - `cd build && ctest` (runs all GoogleTest-based tests)
- **Run:**
  - `./dbus-mqtt-bridge path/to/config.yaml`
- **Simulators:**
  - `build/dbus-simulator` and `build/mqtt-simulator` for local integration testing

## Conventions & Patterns
- **C++20** throughout; prefer modern idioms (smart pointers, ranges, structured bindings)
- **Error Handling:** Use exceptions for fatal errors, return status for recoverable issues
- **Config:** All runtime options and mappings are YAML-driven (see `Config.cpp`)
- **Testing:** All new logic should be covered in `tests/` using GoogleTest
- **External Integration:**
  - DBus via `sdbus-c++` (system/session bus selectable)
  - MQTT via `paho-mqtt-cpp`
  - JSON via `nlohmann-json`

## Key Files
- `src/Bridge.cpp`, `src/DbusManager.cpp`, `src/MqttManager.cpp`, `src/Config.cpp`
- `include/Bridge.hpp`, etc.
- `build/config.yaml` (example config)
- `tests/` (unit tests)

## Tips for AI Agents
- Always check `Config.cpp` and `build/config.yaml` for runtime behavior and mappings
- When adding features, update both C++ logic and YAML config as needed
- Use the simulators for end-to-end testing without real DBus/MQTT infrastructure
- Keep build/test instructions up to date in this file and `README.md`
