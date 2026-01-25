cmake_minimum_required(VERSION 3.20)
project(dbus-mqtt-bridge VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(eclipse-paho-mqtt-c REQUIRED)
find_package(PahoMqttCpp REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(nlohmann_json 3.0 REQUIRED)

pkg_check_modules(SDBUS_CPP REQUIRED IMPORTED_TARGET sdbus-c++)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Main executable
add_executable(dbus-mqtt-bridge
    src/main.cpp
    src/Config.cpp
    src/ConfigValidator.cpp
    src/Bridge.cpp
    src/DbusManager.cpp
    src/MqttManager.cpp
)

# Link libraries
target_link_libraries(dbus-mqtt-bridge
    PkgConfig::SDBUS_CPP
    paho-mqttpp3
    paho-mqtt3as
    yaml-cpp
    nlohmann_json::nlohmann_json
)

# Install targets
install(TARGETS dbus-mqtt-bridge DESTINATION bin)

# Install systemd service file
install(FILES ${CMAKE_SOURCE_DIR}/dbus-mqtt-bridge.service 
        DESTINATION lib/systemd/system)

# Install example config
install(FILES ${CMAKE_SOURCE_DIR}/examples/config.yaml 
        DESTINATION share/doc/dbus-mqtt-bridge/examples)

# Install D-Bus policy example
install(FILES ${CMAKE_SOURCE_DIR}/examples/dbus-policy.conf 
        DESTINATION share/doc/dbus-mqtt-bridge/examples)

# Install documentation
install(FILES 
        ${CMAKE_SOURCE_DIR}/README.md 
        ${CMAKE_SOURCE_DIR}/LICENSE 
        DESTINATION share/doc/dbus-mqtt-bridge)
