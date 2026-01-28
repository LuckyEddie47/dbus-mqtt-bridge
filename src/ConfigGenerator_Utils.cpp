// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee
// ConfigGenerator utility functions

#include "ConfigGenerator.h"
#include "ConfigValidator.h"
#include "InteractiveSelector.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

std::string ConfigGenerator::configToYaml(const Config& config) {
    std::ostringstream oss;
    
    oss << "mqtt:" << std::endl;
    oss << "  broker: " << config.mqtt.broker << std::endl;
    oss << "  port: " << config.mqtt.port << std::endl;
    
    if (!config.mqtt.username.empty()) {
        oss << "  auth:" << std::endl;
        oss << "    username: " << config.mqtt.username << std::endl;
        oss << "    password: " << config.mqtt.password << std::endl;
    }
    
    oss << std::endl;
    oss << "bus_type: " << config.bus_type << std::endl;
    oss << std::endl;
    
    oss << "mappings:" << std::endl;
    
    // D-Bus to MQTT mappings
    oss << "  dbus_to_mqtt:" << std::endl;
    if (config.dbus_to_mqtt.empty()) {
        oss << "    []" << std::endl;
    } else {
        for (const auto& m : config.dbus_to_mqtt) {
            oss << "    - service: " << m.service << std::endl;
            oss << "      path: " << m.path << std::endl;
            oss << "      interface: " << m.interface << std::endl;
            oss << "      signal: " << m.signal << std::endl;
            oss << "      topic: " << m.topic << std::endl;
        }
    }
    
    // MQTT to D-Bus mappings
    oss << "  mqtt_to_dbus:" << std::endl;
    if (config.mqtt_to_dbus.empty()) {
        oss << "    []" << std::endl;
    } else {
        for (const auto& m : config.mqtt_to_dbus) {
            oss << "    - topic: " << m.topic << std::endl;
            oss << "      service: " << m.service << std::endl;
            oss << "      path: " << m.path << std::endl;
            oss << "      interface: " << m.interface << std::endl;
            oss << "      method: " << m.method << std::endl;
        }
    }
    
    return oss.str();
}

void ConfigGenerator::printConfig(const Config& config) {
    std::string yaml = configToYaml(config);
    
    // Print with line numbers
    std::istringstream iss(yaml);
    std::string line;
    int line_num = 1;
    
    while (std::getline(iss, line)) {
        std::cout << std::setw(3) << line_num++ << " | " << line << std::endl;
    }
}

bool ConfigGenerator::saveConfig(const Config& config, const std::string& path) {
    try {
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        file << configToYaml(config);
        file.close();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigGenerator::fixValidationErrors(Config& config) {
    auto validation = config.validate();
    
    if (!validation.hasErrors()) {
        return true;
    }
    
    std::cout << "\nAttempting to fix validation errors interactively...\n" << std::endl;
    
    for (const auto& error : validation.errors) {
        std::cout << "Error in field '" << error.field << "': " << error.message << std::endl;
        
        // Try to fix based on field name
        if (error.field == "mqtt.broker") {
            promptMqttBroker(config);
        } else if (error.field == "mqtt.port") {
            promptMqttPort(config);
        } else if (error.field == "mqtt.auth") {
            promptMqttAuth(config);
        } else if (error.field == "bus_type") {
            configureBusType(config);
        } else if (error.field.find("dbus_to_mqtt") != std::string::npos) {
            // Extract index from field name like "mappings.dbus_to_mqtt[0].service"
            size_t start = error.field.find('[');
            size_t end = error.field.find(']');
            if (start != std::string::npos && end != std::string::npos) {
                try {
                    size_t idx = std::stoul(error.field.substr(start + 1, end - start - 1));
                    if (idx < config.dbus_to_mqtt.size()) {
                        std::cout << "Editing D-Bus → MQTT mapping #" << (idx + 1) << std::endl;
                        editDbusToMqttMapping(config, idx);
                    }
                } catch (...) {}
            }
        } else if (error.field.find("mqtt_to_dbus") != std::string::npos) {
            size_t start = error.field.find('[');
            size_t end = error.field.find(']');
            if (start != std::string::npos && end != std::string::npos) {
                try {
                    size_t idx = std::stoul(error.field.substr(start + 1, end - start - 1));
                    if (idx < config.mqtt_to_dbus.size()) {
                        std::cout << "Editing MQTT → D-Bus mapping #" << (idx + 1) << std::endl;
                        editMqttToDbusMapping(config, idx);
                    }
                } catch (...) {}
            }
        }
    }
    
    // Re-validate
    validation = config.validate();
    
    if (validation.hasErrors()) {
        std::cout << "\nStill has errors after fixes:" << std::endl;
        ConfigValidator::printValidationErrors(validation);
        return false;
    }
    
    std::cout << "\n✓ All errors fixed!" << std::endl;
    return true;
}
