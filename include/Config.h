// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#pragma once

#include <string>
#include <vector>
#include "ConfigValidator.h"

struct MqttConfig {
    std::string broker;
    int port = 1883;
    std::string username;
    std::string password;
};

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

struct Config {
    MqttConfig mqtt;
    std::string bus_type = "system";
    std::vector<DbusToMqttMapping> dbus_to_mqtt;
    std::vector<MqttToDbusMapping> mqtt_to_dbus;

    static Config loadFromFile(const std::string& filename);
    
    // Validation
    ValidationResult validate() const;
    
private:
    ValidationResult validateMqttConfig() const;
    ValidationResult validateMappings() const;
    ValidationResult validateDbusToMqttMapping(const DbusToMqttMapping& mapping, size_t index) const;
    ValidationResult validateMqttToDbusMapping(const MqttToDbusMapping& mapping, size_t index) const;
};
