# Technical Specification: DBus-MQTT Bridge

## Technical Context
- **Language**: C++20
- **Dependencies**:
  - `sdbus-c++`: High-level C++ DBus library (wraps `libsystemd`).
  - `paho.mqtt.cpp`: Eclipse Paho MQTT C++ client library.
  - `yaml-cpp`: For parsing the configuration file.
  - `nlohmann/json`: For serializing/deserializing DBus arguments (to handle "all types").
- **Build System**: CMake
- **Target OS**: Linux (systemd-based)

## Implementation Approach
The service will be a generic bridge that maps DBus signals/methods to MQTT topics based on a YAML configuration.

### 1. Configuration Model
The configuration will define:
- **Broker**: address, port, username, password.
- **Mappings**:
  - `dbus_to_mqtt`: List of DBus signals to monitor and publish to MQTT.
  - `mqtt_to_dbus`: List of MQTT topics to subscribe to and trigger DBus methods.

### 2. DBus to MQTT (Monitoring)
- The bridge will dynamically subscribe to DBus signals defined in the config.
- When a signal is intercepted, its arguments will be serialized to a JSON string (to ensure type transparency) and published to the configured MQTT topic.

### 3. MQTT to DBus (Commands)
- The bridge will subscribe to MQTT topics.
- When a message arrives, it will be parsed. The message should contain the arguments for the DBus method call.
- The bridge will then perform the DBus method call on the target object/interface.

### 4. Transparency
To support "all types", DBus arguments will be converted to/from JSON. `sdbus-c++` provides type-safe access, but since this is generic, we will use its dynamic API or introspection where possible to handle varying signatures.

## Source Code Structure
- `src/main.cpp`: Initialization and main loop.
- `src/Config.hpp/cpp`: Configuration loading and validation.
- `src/DbusManager.hpp/cpp`: Handles DBus connection, signal monitoring, and method calls.
- `src/MqttManager.hpp/cpp`: Handles MQTT connection, publishing, and subscriptions.
- `src/Bridge.hpp/cpp`: Orchestrates the flow between DBus and MQTT.

## Data Model / API
### Configuration Example (config.yaml)
```yaml
mqtt:
  broker: "localhost"
  port: 1883
  auth:
    username: "user"
    password: "password"

mappings:
  dbus_to_mqtt:
    - service: "org.kde.kstars"
      path: "/KStars"
      interface: "org.kde.kstars"
      signal: "newStandardError"
      topic: "kstars/stderr"
  
  mqtt_to_dbus:
    - topic: "kstars/command/openFITS"
      service: "org.kde.kstars"
      path: "/KStars"
      interface: "org.kde.kstars"
      method: "openFITS"
```

## Verification Approach
- **Unit Tests**: Using `GTest` for every module. Each phase will include its own set of unit tests to ensure functional correctness.
- **Test Plan**: A dedicated document `/home/ed/Data/Code/dbus-mqtt-bridge/docs/test_plan.md` will be created to outline all test cases (unit, integration, and E2E).
- **Simulators**:
  - `dbus-simulator`: A standalone C++ program that exposes a mock DBus service to emit signals and respond to methods for E2E testing.
  - `mqtt-simulator`: A standalone C++ (or Python if preferred for simplicity) program to subscribe to/publish MQTT messages and verify the bridge's throughput.
- **Integration Tests**:
  - Use `mosquitto` as the broker.
  - Use `busctl` or the `dbus-simulator` to verify signal capture and method calls.
- **Linting**: `clang-format` and `clang-tidy`.
