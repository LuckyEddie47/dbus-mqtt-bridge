// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#include "Config.h"
#include "ConfigValidator.h"
#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <sstream>

Config Config::loadFromFile(const std::string& filename) {
    YAML::Node node = YAML::LoadFile(filename);
    Config config;

    if (!node["mqtt"]) throw std::runtime_error("Missing 'mqtt' section in config");
    
    auto mqtt = node["mqtt"];
    config.mqtt.broker = mqtt["broker"].as<std::string>();
    config.mqtt.port = mqtt["port"].as<int>(1883);

    if (node["bus_type"]) {
        config.bus_type = node["bus_type"].as<std::string>();
    }

    if (mqtt["auth"]) {
        auto auth = mqtt["auth"];
        if (auth["username"]) config.mqtt.username = auth["username"].as<std::string>();
        if (auth["password"]) config.mqtt.password = auth["password"].as<std::string>();
    }

    if (node["mappings"]) {
        auto mappings = node["mappings"];
        
        if (mappings["dbus_to_mqtt"]) {
            for (auto m : mappings["dbus_to_mqtt"]) {
                config.dbus_to_mqtt.push_back({
                    m["service"].as<std::string>(),
                    m["path"].as<std::string>(),
                    m["interface"].as<std::string>(),
                    m["signal"].as<std::string>(),
                    m["topic"].as<std::string>()
                });
            }
        }

        if (mappings["mqtt_to_dbus"]) {
            for (auto m : mappings["mqtt_to_dbus"]) {
                config.mqtt_to_dbus.push_back({
                    m["topic"].as<std::string>(),
                    m["service"].as<std::string>(),
                    m["path"].as<std::string>(),
                    m["interface"].as<std::string>(),
                    m["method"].as<std::string>()
                });
            }
        }
    }

    return config;
}

ValidationResult Config::validate() const {
    ValidationResult result;
    
    // Validate MQTT configuration
    auto mqtt_result = validateMqttConfig();
    result.errors.insert(result.errors.end(), mqtt_result.errors.begin(), mqtt_result.errors.end());
    result.warnings.insert(result.warnings.end(), mqtt_result.warnings.begin(), mqtt_result.warnings.end());
    if (mqtt_result.hasErrors()) result.valid = false;
    
    // Validate mappings
    auto mapping_result = validateMappings();
    result.errors.insert(result.errors.end(), mapping_result.errors.begin(), mapping_result.errors.end());
    result.warnings.insert(result.warnings.end(), mapping_result.warnings.begin(), mapping_result.warnings.end());
    if (mapping_result.hasErrors()) result.valid = false;
    
    return result;
}

ValidationResult Config::validateMqttConfig() const {
    ValidationResult result;
    
    // Validate broker
    if (mqtt.broker.empty()) {
        result.addError("mqtt.broker", "MQTT broker is required");
    } else if (!ConfigValidator::validateMqttBroker(mqtt.broker)) {
        result.addError("mqtt.broker", 
            "Invalid MQTT broker '" + mqtt.broker + "'. Must be a valid hostname or IP address");
    }
    
    // Validate port
    if (!ConfigValidator::validateMqttPort(mqtt.port)) {
        result.addError("mqtt.port", 
            "Invalid MQTT port " + std::to_string(mqtt.port) + ". Must be between 1 and 65535");
    }
    
    // Validate authentication (both or neither)
    bool has_username = !mqtt.username.empty();
    bool has_password = !mqtt.password.empty();
    if (has_username != has_password) {
        result.addError("mqtt.auth", 
            "Both username and password must be provided together, or neither");
    }
    
    // Validate bus type
    if (!ConfigValidator::validateBusType(bus_type)) {
        result.addError("bus_type", 
            "Invalid bus_type '" + bus_type + "'. Must be 'system' or 'session'");
    }
    
    return result;
}

ValidationResult Config::validateMappings() const {
    ValidationResult result;
    
    // Warn if no mappings defined
    if (dbus_to_mqtt.empty() && mqtt_to_dbus.empty()) {
        result.addWarning("No mappings defined. Service will run but do nothing.");
    }
    
    // Validate each dbus_to_mqtt mapping
    for (size_t i = 0; i < dbus_to_mqtt.size(); ++i) {
        auto mapping_result = validateDbusToMqttMapping(dbus_to_mqtt[i], i);
        result.errors.insert(result.errors.end(), mapping_result.errors.begin(), mapping_result.errors.end());
        result.warnings.insert(result.warnings.end(), mapping_result.warnings.begin(), mapping_result.warnings.end());
        if (mapping_result.hasErrors()) result.valid = false;
    }
    
    // Validate each mqtt_to_dbus mapping
    for (size_t i = 0; i < mqtt_to_dbus.size(); ++i) {
        auto mapping_result = validateMqttToDbusMapping(mqtt_to_dbus[i], i);
        result.errors.insert(result.errors.end(), mapping_result.errors.begin(), mapping_result.errors.end());
        result.warnings.insert(result.warnings.end(), mapping_result.warnings.begin(), mapping_result.warnings.end());
        if (mapping_result.hasErrors()) result.valid = false;
    }
    
    // Check for duplicate MQTT topics in subscriptions
    std::vector<std::string> subscribe_topics;
    for (const auto& mapping : mqtt_to_dbus) {
        if (std::find(subscribe_topics.begin(), subscribe_topics.end(), mapping.topic) != subscribe_topics.end()) {
            result.addError("mappings.mqtt_to_dbus", 
                "Duplicate MQTT topic '" + mapping.topic + "' in mqtt_to_dbus mappings");
        }
        subscribe_topics.push_back(mapping.topic);
    }
    
    return result;
}

ValidationResult Config::validateDbusToMqttMapping(const DbusToMqttMapping& mapping, size_t index) const {
    ValidationResult result;
    std::string prefix = "mappings.dbus_to_mqtt[" + std::to_string(index) + "]";
    
    // Validate service name
    if (!ConfigValidator::validateDbusServiceName(mapping.service)) {
        result.addError(prefix + ".service", 
            "Invalid D-Bus service name '" + mapping.service + 
            "'. Must follow reverse-DNS format (e.g., org.example.Service)");
    }
    
    // Validate object path
    if (!ConfigValidator::validateDbusObjectPath(mapping.path)) {
        result.addError(prefix + ".path", 
            "Invalid D-Bus object path '" + mapping.path + 
            "'. Must start with '/' and contain only [a-zA-Z0-9_/] (e.g., /org/example/Object)");
    }
    
    // Validate interface name
    if (!ConfigValidator::validateDbusInterfaceName(mapping.interface)) {
        result.addError(prefix + ".interface", 
            "Invalid D-Bus interface name '" + mapping.interface + 
            "'. Must follow reverse-DNS format (e.g., org.example.Interface)");
    }
    
    // Validate signal name
    if (!ConfigValidator::validateDbusMemberName(mapping.signal)) {
        result.addError(prefix + ".signal", 
            "Invalid D-Bus signal name '" + mapping.signal + 
            "'. Must start with letter and contain only [a-zA-Z0-9_]");
    }
    
    // Validate MQTT topic (no wildcards in publish topics)
    if (!ConfigValidator::validateMqttTopic(mapping.topic, false)) {
        result.addError(prefix + ".topic", 
            "Invalid MQTT topic '" + mapping.topic + 
            "'. Wildcards (+, #) are not allowed in publish topics");
    }
    
    return result;
}

ValidationResult Config::validateMqttToDbusMapping(const MqttToDbusMapping& mapping, size_t index) const {
    ValidationResult result;
    std::string prefix = "mappings.mqtt_to_dbus[" + std::to_string(index) + "]";
    
    // Validate MQTT topic (wildcards allowed in subscriptions)
    if (!ConfigValidator::validateMqttTopic(mapping.topic, true)) {
        result.addError(prefix + ".topic", 
            "Invalid MQTT topic '" + mapping.topic + "'");
    }
    
    // Warn about wildcard usage
    if (mapping.topic.find('+') != std::string::npos || 
        mapping.topic.find('#') != std::string::npos) {
        result.addWarning("MQTT subscription topic '" + mapping.topic + 
            "' contains wildcards. Ensure this is intended.");
    }
    
    // Validate service name
    if (!ConfigValidator::validateDbusServiceName(mapping.service)) {
        result.addError(prefix + ".service", 
            "Invalid D-Bus service name '" + mapping.service + 
            "'. Must follow reverse-DNS format (e.g., org.example.Service)");
    }
    
    // Validate object path
    if (!ConfigValidator::validateDbusObjectPath(mapping.path)) {
        result.addError(prefix + ".path", 
            "Invalid D-Bus object path '" + mapping.path + 
            "'. Must start with '/' and contain only [a-zA-Z0-9_/]");
    }
    
    // Validate interface name
    if (!ConfigValidator::validateDbusInterfaceName(mapping.interface)) {
        result.addError(prefix + ".interface", 
            "Invalid D-Bus interface name '" + mapping.interface + 
            "'. Must follow reverse-DNS format");
    }
    
    // Validate method name
    if (!ConfigValidator::validateDbusMemberName(mapping.method)) {
        result.addError(prefix + ".method", 
            "Invalid D-Bus method name '" + mapping.method + 
            "'. Must start with letter and contain only [a-zA-Z0-9_]");
    }
    
    return result;
}
