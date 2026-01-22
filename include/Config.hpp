#pragma once

#include <string>
#include <vector>
#include <optional>

struct DbusToMqttMapping {
    std::string service;
    std::string path;
    std::string interface;
    std::string signal;
    std::string topic;
};

struct MqttToDbusMapping {
    std::string topic;
    std::string service;
    std::string path;
    std::string interface;
    std::string method;
};

struct MqttConfig {
    std::string broker;
    int port = 1883;
    std::optional<std::string> username;
    std::optional<std::string> password;
};

struct Config {
    MqttConfig mqtt;
    std::string bus_type = "session"; // Default to session for now to keep tests passing
    std::vector<DbusToMqttMapping> dbus_to_mqtt;
    std::vector<MqttToDbusMapping> mqtt_to_dbus;

    static Config loadFromFile(const std::string& filename);
};
