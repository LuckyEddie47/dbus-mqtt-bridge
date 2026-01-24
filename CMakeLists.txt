cmake_minimum_required(VERSION 3.20)
project(dbus-mqtt-bridge VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use system libraries (proper Debian packaging)
find_package(PkgConfig REQUIRED)

# 1. nlohmann_json - system package
find_package(nlohmann_json 3.0 REQUIRED)

# 2. yaml-cpp - system package
find_package(yaml-cpp REQUIRED)

# 3. sdbus-c++ - system package
pkg_check_modules(SDBUS REQUIRED IMPORTED_TARGET sdbus-c++)

# 4. libsystemd - system package (required by sdbus-c++)
pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)

# 5. Paho MQTT - use CMake config files
find_package(eclipse-paho-mqtt-c REQUIRED)
find_package(PahoMqttCpp REQUIRED)

# Main Executable
add_executable(${PROJECT_NAME} src/main.cpp src/Config.cpp src/DbusManager.cpp src/MqttManager.cpp src/Bridge.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        PkgConfig::SDBUS
        PahoMqttCpp::paho-mqttpp3
        yaml-cpp
        nlohmann_json::nlohmann_json
)

# Installation rules
include(GNUInstallDirs)

# Install main executable
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install systemd service file
install(FILES dbus-mqtt-bridge.service
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/system
)

# Install example configuration
install(FILES examples/config.yaml
    DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
)

# Install example D-Bus policy
install(FILES examples/dbus-policy.conf
    DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
)

# Install documentation
install(FILES README.md LICENSE
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
    OPTIONAL
)