# dbus-mqtt-bridge(1)

Copyright (C) 2026 Ed Lee
License: GPL-3.0-or-later
Author: Ed Lee

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

## NAME

dbus-mqtt-bridge - Bi-directional bridge between DBus and MQTT

## SYNOPSIS

dbus-mqtt-bridge [config.yaml]

## DESCRIPTION

dbus-mqtt-bridge is a high-performance Linux system service that bridges DBus (Linux IPC) and MQTT (IoT messaging) in both directions. It enables integration between local Linux services and remote IoT/cloud systems using a YAML-driven configuration.

## BUILDING

### Prerequisites
- C++20 compiler (GCC 10+ recommended)
- CMake 3.20+
- libsystemd-dev (for DBus via sdbus-c++)
- pkg-config

### Build Steps
```
git clone <repo-url>
cd dbus-mqtt-bridge
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## INSTALLATION

### Manual Installation
- The main binary is at `build/dbus-mqtt-bridge`.
- Optionally, copy it to `/usr/local/bin`:
  ```
  sudo cp dbus-mqtt-bridge /usr/local/bin/
  ```
- For systemd integration:
  ```
  sudo cp ../dbus-mqtt-bridge.service /etc/systemd/system/
  sudo systemctl daemon-reload
  sudo systemctl enable dbus-mqtt-bridge
  sudo systemctl start dbus-mqtt-bridge
  ```

## CONFIGURATION

All runtime options and bus/topic mappings are defined in a YAML file. See `build/config.yaml` for a full example. Pass the config file as an argument:
```
dbus-mqtt-bridge path/to/config.yaml
```

### Example config.yaml
```yaml
mqtt:
  host: "localhost"
  port: 1883
  username: "user"
  password: "pass"
dbus:
  bus: "session"  # or "system"
mappings:
  - dbus:
      service: "com.example.Service"
      path: "/com/example/Object"
      interface: "com.example.Interface"
      signal: "ExampleSignal"
    mqtt:
      topic: "example/signal"
  - mqtt:
      topic: "example/call"
    dbus:
      service: "com.example.Service"
      path: "/com/example/Object"
      interface: "com.example.Interface"
      method: "ExampleMethod"
```

## TESTING

- Use `build/dbus-simulator` and `build/mqtt-simulator` for local integration testing.
- Run all unit tests:
  ```
  cd build
  ctest
  ```

## TROUBLESHOOTING

- Check logs for errors if the bridge fails to start.
- Ensure DBus and MQTT endpoints are reachable and credentials are correct.
- Validate your YAML config with a linter if startup fails due to configuration errors.

## FILES

- Main source: `src/Bridge.cpp`, `src/DbusManager.cpp`, `src/MqttManager.cpp`, `src/Config.cpp`
- Example config: `build/config.yaml`
- Service file: `dbus-mqtt-bridge.service`
- Simulators: `build/dbus-simulator`, `build/mqtt-simulator`
