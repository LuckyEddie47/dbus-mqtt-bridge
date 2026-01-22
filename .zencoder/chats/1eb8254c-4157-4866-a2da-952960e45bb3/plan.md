# Spec and build

## Agent Instructions

Ask the user questions when anything is unclear or needs their input. This includes:

- Ambiguous or incomplete requirements
- Technical decisions that affect architecture or user experience
- Trade-offs that require business context

Do not make assumptions on important decisions — get clarification first.

---

## Workflow Steps

### [x] Step: Technical Specification

Assess the task's difficulty, as underestimating it leads to poor outcomes.

- **Complexity**: Hard (C++, asynchronous bridging, dynamic DBus typing)
- **Technical Context**: C++20, sdbus-c++, Paho MQTT C++, yaml-cpp, nlohmann-json.

---

### [ ] Step: Implementation

#### 1. Project Setup [x]
- Initialize CMake project.
- Set up dependency management.
- Create directory structure.
- **Verification**: `cmake ..` succeeds.

#### 2. Test Plan & Simulators [x]
- Create `/home/ed/Data/Code/dbus-mqtt-bridge/docs/test_plan.md`.
- Implement `dbus-simulator`.
- Implement `mqtt-simulator`.
- **Verification**: Simulators run and communicate with their respective buses.

#### 3. Configuration Management [x]
- Implement YAML configuration parser.
- **Unit Tests**: Test valid/invalid YAML, missing fields, type mismatches.
- **Verification**: `make test` for config module.

#### 4. DBus Manager [ ]
- Implement DBus connection and dynamic signal monitoring.
- Implement method call execution.
- **Unit Tests**: Mock DBus calls and verify handlers.
- **Verification**: `make test` for DBus module.

#### 5. MQTT Manager [ ]
- Implement MQTT connection, auth, publishing, and subscription.
- **Unit Tests**: Mock MQTT broker/client and verify pub/sub logic.
- **Verification**: `make test` for MQTT module.

#### 6. Bridge Logic & End-to-End [ ]
- Implement DBus-to-MQTT signal forwarding (JSON).
- Implement MQTT-to-DBus command processing (JSON).
- **Unit Tests**: Verify JSON transformation logic.
- **E2E Testing**: Run bridge with both simulators and a Mosquitto broker.
- **Verification**: End-to-end message flow confirmed.

#### 7. System Integration [ ]
- Create systemd service unit file.
- Perform final manual verification.

---

### [ ] Step: Final Report
- Write report to `/home/ed/Data/Code/dbus-mqtt-bridge/.zencoder/chats/1eb8254c-4157-4866-a2da-952960e45bb3/report.md`.
