.TH DBUS-MQTT-BRIDGE 1 "January 2026" "dbus-mqtt-bridge" "User Commands"

.SH NAME
dbus-mqtt-bridge \- Bi-directional bridge between DBus and MQTT

.SH SYNOPSIS
.B dbus-mqtt-bridge
\fIconfig.yaml\fR

.SH DESCRIPTION
\fBdbus-mqtt-bridge\fR is a high-performance Linux system service that bridges DBus (Linux IPC) and MQTT (IoT messaging) in both directions. It enables integration between local Linux services and remote IoT/cloud systems using a YAML-driven configuration.

.SH BUILDING
.SS Prerequisites
.RS
- C++20 compiler (GCC 10+ recommended)
- CMake 3.20+
- libsystemd-dev (for DBus via sdbus-c++)
- pkg-config
.RE

.SS Build Steps
.nf
git clone <repo-url>
cd dbus-mqtt-bridge
mkdir build && cd build
cmake ..
make -j$(nproc)
.fi

.SH INSTALLATION
.SS Manual Installation
.RS
- The main binary is at \fBbuild/dbus-mqtt-bridge\fR.
- Optionally, copy it to /usr/local/bin:
.nf
sudo cp dbus-mqtt-bridge /usr/local/bin/
.fi
- For systemd integration:
.nf
sudo cp ../dbus-mqtt-bridge.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable dbus-mqtt-bridge
sudo systemctl start dbus-mqtt-bridge
.fi
.RE

.SH CONFIGURATION
All runtime options and bus/topic mappings are defined in a YAML file. See \fBbuild/config.yaml\fR for a full example. Pass the config file as an argument:
.nf
dbus-mqtt-bridge path/to/config.yaml
.fi

.SS Example config.yaml
.nf
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
.fi

.SH TESTING
.RS
- Use \fBbuild/dbus-simulator\fR and \fBbuild/mqtt-simulator\fR for local integration testing.
- Run all unit tests:
.nf
cd build
ctest
.fi
.RE

.SH TROUBLESHOOTING
.RS
- Check logs for errors if the bridge fails to start.
- Ensure DBus and MQTT endpoints are reachable and credentials are correct.
- Validate your YAML config with a linter if startup fails due to configuration errors.
.RE

.SH FILES
.RS
- Main source: \fBsrc/Bridge.cpp\fR, \fBsrc/DbusManager.cpp\fR, \fBsrc/MqttManager.cpp\fR, \fBsrc/Config.cpp\fR
- Example config: \fBbuild/config.yaml\fR
- Service file: \fBdbus-mqtt-bridge.service\fR
- Simulators: \fBbuild/dbus-simulator\fR, \fBbuild/mqtt-simulator\fR
.RE
