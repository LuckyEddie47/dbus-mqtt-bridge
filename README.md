# DBus-MQTT Bridge

A high-performance, lightweight Linux system service that acts as a bi-directional bridge between DBus signals/methods and MQTT topics.

## Dependencies

The project uses `FetchContent` for most dependencies, but requires the following on the host system:
- **C++20 Compiler** (e.g., GCC 10+)
- **CMake** 3.20+
- **libsystemd-dev** (for DBus communication via `sdbus-c++`)
- **pkg-config**

Internal dependencies managed by CMake:
- `sdbus-c++`
- `paho-mqtt-cpp` (and `paho-mqtt-c`)
- `nlohmann-json`
- `yaml-cpp`
- `googletest`

## Build Guide

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Running Tests
```bash
ctest
```

## Usage

Configure the bridge using a YAML file (see `build/config.yaml` for an example):

```bash
./dbus-mqtt-bridge path/to/config.yaml
```

## Limitations

- **Complex Types**: Currently supports basic DBus types (strings, integers, booleans, doubles). Nested containers (arrays of arrays, dictionaries) are not yet fully implemented in the JSON transformation layer.
- **JSON Format**: DBus signals are serialized as JSON arrays of their arguments. MQTT-to-DBus commands expect a JSON array matching the method's signature.
- **Bus Type**: Defaults to the Session Bus for development; can be configured to use the System Bus in `config.yaml`.
