// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#include "ConfigGenerator.h"
#include "ConfigValidator.h"
#include "InteractiveSelector.h"
#include "DbusIntrospector.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <yaml-cpp/yaml.h>

int ConfigGenerator::run(int argc, char** argv) {
    std::string from_file;
    std::string output_file;
    
    if (!parseGeneratorArgs(argc, argv, from_file, output_file)) {
        return 1;
    }
    
    std::cout << "\n=== D-Bus to MQTT Bridge - Configuration Generator ===\n" << std::endl;
    
    // Load existing config if specified
    Config config;
    if (!from_file.empty()) {
        std::cout << "Loading existing configuration from " << from_file << "..." << std::endl;
        auto partial = loadPartialConfig(from_file);
        if (partial) {
            config = *partial;
            std::cout << "Loaded existing configuration. Will prompt for missing/invalid fields.\n" << std::endl;
        } else {
            std::cout << "Could not load config file. Starting fresh.\n" << std::endl;
        }
    } else {
        std::cout << "Starting fresh configuration.\n" << std::endl;
    }
    
    // Interactive configuration
    configureMqtt(config);
    configureBusType(config);
    configureMappings(config);
    
    // Validate final config
    std::cout << "\n--- Validating Configuration ---\n" << std::endl;
    auto validation = config.validate();
    
    if (validation.hasErrors()) {
        std::cout << "Configuration has errors:\n" << std::endl;
        ConfigValidator::printValidationErrors(validation);
        
        if (InteractiveSelector::promptYesNo("\nWould you like to fix these errors?", true)) {
            if (!fixValidationErrors(config)) {
                std::cout << "Configuration still has errors. Exiting." << std::endl;
                return 1;
            }
        } else {
            std::cout << "Configuration not saved due to errors." << std::endl;
            return 1;
        }
    }
    
    if (validation.hasWarnings()) {
        std::cout << "Warnings:" << std::endl;
        for (const auto& warning : validation.warnings) {
            std::cout << "  - " << warning << std::endl;
        }
        std::cout << std::endl;
    }
    
    std::cout << "✓ Configuration is valid!\n" << std::endl;
    
    // Show final config
    std::cout << "--- Final Configuration ---\n" << std::endl;
    printConfig(config);
    
    // Save config
    std::cout << "\n--- Save Configuration ---\n" << std::endl;
    
    std::string save_path = output_file;
    if (save_path.empty()) {
        save_path = InteractiveSelector::promptText(
            "Enter output path (or press Enter for stdout)",
            ""
        );
    }
    
    if (save_path.empty()) {
        std::cout << "\nConfiguration output:\n" << std::endl;
        std::cout << "---" << std::endl;
        std::cout << configToYaml(config);
        std::cout << "---" << std::endl;
    } else {
        if (saveConfig(config, save_path)) {
            std::cout << "✓ Configuration saved to: " << save_path << std::endl;
            
            // Show next steps
            if (config.bus_type == "system") {
                std::cout << "\nNext steps:" << std::endl;
                std::cout << "  1. Review D-Bus policy requirements" << std::endl;
                std::cout << "  2. Install config: sudo mv " << save_path << " /etc/dbus-mqtt-bridge/config.yaml" << std::endl;
                std::cout << "  3. Configure D-Bus policy: /etc/dbus-1/system.d/dbus-mqtt-bridge.conf" << std::endl;
                std::cout << "  4. Start service: sudo systemctl enable --now dbus-mqtt-bridge" << std::endl;
            } else {
                std::cout << "\nNext steps:" << std::endl;
                std::cout << "  1. Run as user service: systemctl --user enable --now dbus-mqtt-bridge" << std::endl;
                std::cout << "  2. Or run manually: dbus-mqtt-bridge " << save_path << std::endl;
            }
        } else {
            std::cout << "Error: Could not write to " << save_path << std::endl;
            std::cout << "\nOutput would be:" << std::endl;
            std::cout << "---" << std::endl;
            std::cout << configToYaml(config);
            std::cout << "---" << std::endl;
        }
    }
    
    return 0;
}

bool ConfigGenerator::parseGeneratorArgs(int argc, char** argv,
                                         std::string& from_file,
                                         std::string& output_file) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--from" && i + 1 < argc) {
            from_file = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "--generate-config") {
            // This is expected, skip
            continue;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            std::cerr << "Usage: dbus-mqtt-bridge --generate-config [--from FILE] [-o OUTPUT]" << std::endl;
            return false;
        }
    }
    
    return true;
}

std::optional<Config> ConfigGenerator::loadPartialConfig(const std::string& path) {
    try {
        return Config::loadFromFile(path);
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not load config: " << e.what() << std::endl;
        return std::nullopt;
    }
}

void ConfigGenerator::configureMqtt(Config& config) {
    std::cout << "--- MQTT Configuration ---\n" << std::endl;
    
    promptMqttBroker(config);
    promptMqttPort(config);
    promptMqttAuth(config);
}

void ConfigGenerator::configureBusType(Config& config) {
    std::cout << "\n--- D-Bus Configuration ---\n" << std::endl;
    
    // Set default if not already set
    if (config.bus_type.empty()) {
        config.bus_type = "system";
    }
    
    std::cout << "Default bus type: " << config.bus_type << std::endl;
    std::cout << "Note: Bus type will be adjusted based on selected D-Bus services.\n" << std::endl;
}

void ConfigGenerator::configureMappings(Config& config) {
    std::cout << "\n--- Mappings Configuration ---\n" << std::endl;
    
    std::cout << "Mappings define how D-Bus signals/methods connect to MQTT topics.\n" << std::endl;
    
    manageDbusToMqttMappings(config);
    manageMqttToDbusMapping(config);
}

void ConfigGenerator::promptMqttBroker(Config& config) {
    std::string current = config.mqtt.broker;
    
    while (true) {
        std::string broker = InteractiveSelector::promptText(
            "Enter MQTT broker hostname or IP",
            current.empty() ? "localhost" : current
        );
        
        if (ConfigValidator::validateMqttBroker(broker)) {
            config.mqtt.broker = broker;
            break;
        } else {
            std::cout << "Invalid broker. Must be a valid hostname or IP address." << std::endl;
        }
    }
}

void ConfigGenerator::promptMqttPort(Config& config) {
    int current = config.mqtt.port > 0 ? config.mqtt.port : 1883;
    
    while (true) {
        std::string port_str = InteractiveSelector::promptText(
            "Enter MQTT port",
            std::to_string(current)
        );
        
        try {
            int port = std::stoi(port_str);
            if (ConfigValidator::validateMqttPort(port)) {
                config.mqtt.port = port;
                break;
            } else {
                std::cout << "Invalid port. Must be between 1 and 65535." << std::endl;
            }
        } catch (...) {
            std::cout << "Invalid number." << std::endl;
        }
    }
}

void ConfigGenerator::promptMqttAuth(Config& config) {
    if (InteractiveSelector::promptYesNo("Enable MQTT authentication?", 
                                        !config.mqtt.username.empty())) {
        config.mqtt.username = InteractiveSelector::promptText("Enter MQTT username", "");
        config.mqtt.password = InteractiveSelector::promptPassword("Enter MQTT password");
    } else {
        config.mqtt.username = "";
        config.mqtt.password = "";
    }
}

// Continue in next artifact due to length...
