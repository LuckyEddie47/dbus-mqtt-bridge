cmake_minimum_required(VERSION 3.20)
project(dbus-mqtt-bridge VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
include(CTest)
enable_testing()

# Dependency Management using FetchContent
include(FetchContent)

# 1. nlohmann_json
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

# 2. yaml-cpp (Try system first)
find_package(yaml-cpp QUIET)
if(NOT yaml-cpp_FOUND)
    FetchContent_Declare(
        yaml-cpp
        URL https://github.com/jbeder/yaml-cpp/archive/refs/tags/0.8.0.tar.gz
    )
    FetchContent_MakeAvailable(yaml-cpp)
endif()

# 3. sdbus-c++ (Requires libsystemd on host)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)

FetchContent_Declare(
    sdbus-c++
    URL https://github.com/Kistler-Group/sdbus-cpp/archive/refs/tags/v1.5.0.tar.gz
)
FetchContent_MakeAvailable(sdbus-c++)

# 4. Paho MQTT C and C++
set(PAHO_BUILD_STATIC ON CACHE INTERNAL "")
set(PAHO_WITH_SSL OFF CACHE INTERNAL "") 
set(PAHO_BUILD_SAMPLES OFF CACHE INTERNAL "")
set(PAHO_MQTT_C_BUILD_SAMPLES OFF CACHE INTERNAL "")
set(PAHO_MQTT_CPP_BUILD_SAMPLES OFF CACHE INTERNAL "")
set(PAHO_MQTT_C_INSTALL OFF CACHE INTERNAL "")
set(PAHO_MQTT_CPP_INSTALL OFF CACHE INTERNAL "")

FetchContent_Declare(
    paho-mqtt-c
    URL https://github.com/eclipse/paho.mqtt.c/archive/refs/tags/v1.3.13.tar.gz
)
FetchContent_Populate(paho-mqtt-c)
add_subdirectory(${paho-mqtt-c_SOURCE_DIR} ${paho-mqtt-c_BINARY_DIR} EXCLUDE_FROM_ALL)

FetchContent_Declare(
    paho-mqtt-cpp
    URL https://github.com/eclipse/paho.mqtt.cpp/archive/refs/tags/v1.3.2.tar.gz
)
FetchContent_Populate(paho-mqtt-cpp)
# Patch paho-mqtt-cpp to avoid export/install errors in FetchContent
file(WRITE ${paho-mqtt-cpp_SOURCE_DIR}/cmake/CMakeLists.txt "")
# Force paho-mqtt-cpp to use the paho-mqtt-c targets we just created
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
add_subdirectory(${paho-mqtt-cpp_SOURCE_DIR} ${paho-mqtt-cpp_BINARY_DIR} EXCLUDE_FROM_ALL)

# 5. GTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Main Executable
add_executable(${PROJECT_NAME} src/main.cpp src/Config.cpp src/DbusManager.cpp src/MqttManager.cpp src/Bridge.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        sdbus-c++
        paho-mqttpp3-static
        yaml-cpp
        nlohmann_json::nlohmann_json
)

# Simulators
add_executable(dbus-simulator simulators/dbus_simulator.cpp src/Config.cpp)
target_include_directories(dbus-simulator PRIVATE include)
target_link_libraries(dbus-simulator PRIVATE sdbus-c++ yaml-cpp nlohmann_json::nlohmann_json)

add_executable(mqtt-simulator simulators/mqtt_simulator.cpp src/Config.cpp)
target_include_directories(mqtt-simulator PRIVATE include)
target_link_libraries(mqtt-simulator PRIVATE paho-mqttpp3-static yaml-cpp nlohmann_json::nlohmann_json)

# Unit Tests
add_executable(test_config tests/test_config.cpp src/Config.cpp)
target_include_directories(test_config PRIVATE include)
target_link_libraries(test_config PRIVATE GTest::gtest_main yaml-cpp)
add_test(NAME ConfigTest COMMAND test_config)

add_executable(test_dbus tests/test_dbus.cpp src/DbusManager.cpp src/Config.cpp)
target_include_directories(test_dbus PRIVATE include)
target_link_libraries(test_dbus PRIVATE GTest::gtest_main sdbus-c++ yaml-cpp nlohmann_json::nlohmann_json)
add_test(NAME DbusTest COMMAND test_dbus)

add_executable(test_mqtt tests/test_mqtt.cpp src/MqttManager.cpp src/Config.cpp)
target_include_directories(test_mqtt PRIVATE include)
target_link_libraries(test_mqtt PRIVATE GTest::gtest_main paho-mqttpp3-static yaml-cpp nlohmann_json::nlohmann_json)
add_test(NAME MqttTest COMMAND test_mqtt)
