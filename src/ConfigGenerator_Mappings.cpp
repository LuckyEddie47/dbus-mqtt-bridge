// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee
// ConfigGenerator mapping management functions

#include "ConfigGenerator.h"
#include "ConfigValidator.h"
#include "InteractiveSelector.h"
#include "DbusIntrospector.h"
#include <iostream>
#include <sstream>

void ConfigGenerator::manageDbusToMqttMappings(Config& config) {
    std::cout << "D-Bus to MQTT mappings (D-Bus signals to MQTT topics)\n" << std::endl;
    
    while (true) {
        // Show current mappings
        if (config.dbus_to_mqtt.empty()) {
            std::cout << "No D-Bus to MQTT mappings defined.\n" << std::endl;
        } else {
            std::cout << "Current mappings:" << std::endl;
            for (size_t i = 0; i < config.dbus_to_mqtt.size(); ++i) {
                const auto& m = config.dbus_to_mqtt[i];
                std::cout << "  [" << (i+1) << "] " << m.service << "::" << m.signal 
                         << " -> " << m.topic << std::endl;
            }
            std::cout << std::endl;
        }
        
        // Options menu
        std::vector<std::string> options = {
            "[a] Add new mapping",
            "[c] Continue to next section"
        };
        
        if (!config.dbus_to_mqtt.empty()) {
            options.insert(options.begin() + 1, "[e] Edit mapping");
            options.insert(options.begin() + 2, "[d] Delete mapping");
        }
        
        auto choice = InteractiveSelector::selectFromList(
            "D-Bus to MQTT Mapping Options:",
            options,
            false
        );
        
        if (!choice) break;
        
        if (choice->find("[a]") != std::string::npos) {
            addDbusToMqttMapping(config);
        } else if (choice->find("[e]") != std::string::npos) {
            std::string idx_str = InteractiveSelector::promptText("Enter mapping number to edit", "");
            try {
                size_t idx = std::stoul(idx_str) - 1;
                if (idx < config.dbus_to_mqtt.size()) {
                    editDbusToMqttMapping(config, idx);
                }
            } catch (...) {}
        } else if (choice->find("[d]") != std::string::npos) {
            std::string idx_str = InteractiveSelector::promptText("Enter mapping number to delete", "");
            try {
                size_t idx = std::stoul(idx_str) - 1;
                if (idx < config.dbus_to_mqtt.size()) {
                    deleteDbusToMqttMapping(config, idx);
                }
            } catch (...) {}
        } else if (choice->find("[c]") != std::string::npos) {
            break;
        }
    }
}

void ConfigGenerator::manageMqttToDbusMapping(Config& config) {
    std::cout << "\nMQTT to D-Bus mappings (MQTT topics to D-Bus methods)\n" << std::endl;
    
    while (true) {
        // Show current mappings
        if (config.mqtt_to_dbus.empty()) {
            std::cout << "No MQTT to D-Bus mappings defined.\n" << std::endl;
        } else {
            std::cout << "Current mappings:" << std::endl;
            for (size_t i = 0; i < config.mqtt_to_dbus.size(); ++i) {
                const auto& m = config.mqtt_to_dbus[i];
                std::cout << "  [" << (i+1) << "] " << m.topic 
                         << " -> " << m.service << "::" << m.method << std::endl;
            }
            std::cout << std::endl;
        }
        
        // Options menu
        std::vector<std::string> options = {
            "[a] Add new mapping",
            "[c] Continue"
        };
        
        if (!config.mqtt_to_dbus.empty()) {
            options.insert(options.begin() + 1, "[e] Edit mapping");
            options.insert(options.begin() + 2, "[d] Delete mapping");
        }
        
        auto choice = InteractiveSelector::selectFromList(
            "MQTT to D-Bus Mapping Options:",
            options,
            false
        );
        
        if (!choice) break;
        
        if (choice->find("[a]") != std::string::npos) {
            addMqttToDbusMapping(config);
        } else if (choice->find("[e]") != std::string::npos) {
            std::string idx_str = InteractiveSelector::promptText("Enter mapping number to edit", "");
            try {
                size_t idx = std::stoul(idx_str) - 1;
                if (idx < config.mqtt_to_dbus.size()) {
                    editMqttToDbusMapping(config, idx);
                }
            } catch (...) {}
        } else if (choice->find("[d]") != std::string::npos) {
            std::string idx_str = InteractiveSelector::promptText("Enter mapping number to delete", "");
            try {
                size_t idx = std::stoul(idx_str) - 1;
                if (idx < config.mqtt_to_dbus.size()) {
                    deleteMqttToDbusMapping(config, idx);
                }
            } catch (...) {}
        } else if (choice->find("[c]") != std::string::npos) {
            break;
        }
    }
}

void ConfigGenerator::addDbusToMqttMapping(Config& config) {
    std::cout << "\n--- Add D-Bus to MQTT Mapping ---\n" << std::endl;
    
    DbusToMqttMapping mapping;
    bool system_bus = (config.bus_type == "system");
    
    enum State { SERVICE, PATH, INTERFACE, SIGNAL, TOPIC, DONE, CANCELLED };
    State state = SERVICE;
    
    while (state != DONE && state != CANCELLED) {
        switch (state) {
            case SERVICE:
                if (promptDbusService(mapping.service, mapping.service, system_bus)) {
                    state = PATH;
                } else {
                    state = CANCELLED;
                }
                break;
                
            case PATH:
                if (promptDbusPath(mapping.path, mapping.service, mapping.path, system_bus)) {
                    state = INTERFACE;
                } else {
                    state = SERVICE;
                }
                break;
                
            case INTERFACE:
                if (promptDbusInterface(mapping.interface, mapping.service, mapping.path, mapping.interface, system_bus)) {
                    state = SIGNAL;
                } else {
                    state = PATH;
                }
                break;
                
            case SIGNAL:
                if (promptDbusSignal(mapping.signal, mapping.service, mapping.path, mapping.interface, mapping.signal, system_bus)) {
                    state = TOPIC;
                } else {
                    state = INTERFACE;
                }
                break;
                
            case TOPIC:
                if (promptMqttTopic(mapping.topic, mapping.topic, false)) {
                    state = DONE;
                } else {
                    state = SIGNAL;
                }
                break;
                
            case DONE:
            case CANCELLED:
                break;
        }
    }
    
    if (state == DONE) {
        config.dbus_to_mqtt.push_back(mapping);
        std::cout << "\n✓ Mapping added successfully.\n" << std::endl;
    } else {
        std::cout << "\nMapping cancelled.\n" << std::endl;
    }
}

void ConfigGenerator::addMqttToDbusMapping(Config& config) {
    std::cout << "\n--- Add MQTT to D-Bus Mapping ---\n" << std::endl;
    
    MqttToDbusMapping mapping;
    bool system_bus = (config.bus_type == "system");
    
    enum State { TOPIC, SERVICE, PATH, INTERFACE, METHOD, DONE, CANCELLED };
    State state = TOPIC;
    
    while (state != DONE && state != CANCELLED) {
        switch (state) {
            case TOPIC:
                if (promptMqttTopic(mapping.topic, mapping.topic, true)) {
                    state = SERVICE;
                } else {
                    state = CANCELLED;
                }
                break;
                
            case SERVICE:
                if (promptDbusService(mapping.service, mapping.service, system_bus)) {
                    state = PATH;
                } else {
                    state = TOPIC;
                }
                break;
                
            case PATH:
                if (promptDbusPath(mapping.path, mapping.service, mapping.path, system_bus)) {
                    state = INTERFACE;
                } else {
                    state = SERVICE;
                }
                break;
                
            case INTERFACE:
                if (promptDbusInterface(mapping.interface, mapping.service, mapping.path, mapping.interface, system_bus)) {
                    state = METHOD;
                } else {
                    state = PATH;
                }
                break;
                
            case METHOD:
                if (promptDbusMethod(mapping.method, mapping.service, mapping.path, mapping.interface, mapping.method, system_bus)) {
                    state = DONE;
                } else {
                    state = INTERFACE;
                }
                break;
                
            case DONE:
            case CANCELLED:
                break;
        }
    }
    
    if (state == DONE) {
        config.mqtt_to_dbus.push_back(mapping);
        std::cout << "\n✓ Mapping added successfully.\n" << std::endl;
    } else {
        std::cout << "\nMapping cancelled.\n" << std::endl;
    }
}

void ConfigGenerator::editDbusToMqttMapping(Config& config, size_t index) {
    auto& mapping = config.dbus_to_mqtt[index];
    bool system_bus = (config.bus_type == "system");
    
    std::cout << "\nEditing mapping: " << mapping.service << "::" << mapping.signal 
              << " -> " << mapping.topic << "\n" << std::endl;
    
    enum State { SERVICE, PATH, INTERFACE, SIGNAL, TOPIC, DONE, CANCELLED };
    State state = SERVICE;
    
    while (state != DONE && state != CANCELLED) {
        switch (state) {
            case SERVICE:
                if (promptDbusService(mapping.service, mapping.service, system_bus)) {
                    state = PATH;
                } else {
                    state = CANCELLED;
                }
                break;
                
            case PATH:
                if (promptDbusPath(mapping.path, mapping.service, mapping.path, system_bus)) {
                    state = INTERFACE;
                } else {
                    state = SERVICE;
                }
                break;
                
            case INTERFACE:
                if (promptDbusInterface(mapping.interface, mapping.service, mapping.path, mapping.interface, system_bus)) {
                    state = SIGNAL;
                } else {
                    state = PATH;
                }
                break;
                
            case SIGNAL:
                if (promptDbusSignal(mapping.signal, mapping.service, mapping.path, mapping.interface, mapping.signal, system_bus)) {
                    state = TOPIC;
                } else {
                    state = INTERFACE;
                }
                break;
                
            case TOPIC:
                if (promptMqttTopic(mapping.topic, mapping.topic, false)) {
                    state = DONE;
                } else {
                    state = SIGNAL;
                }
                break;
                
            case DONE:
            case CANCELLED:
                break;
        }
    }
    
    if (state == DONE) {
        std::cout << "\n✓ Mapping updated.\n" << std::endl;
    } else {
        std::cout << "\nEdit cancelled.\n" << std::endl;
    }
}

void ConfigGenerator::editMqttToDbusMapping(Config& config, size_t index) {
    auto& mapping = config.mqtt_to_dbus[index];
    bool system_bus = (config.bus_type == "system");
    
    std::cout << "\nEditing mapping: " << mapping.topic 
              << " -> " << mapping.service << "::" << mapping.method << "\n" << std::endl;
    
    enum State { TOPIC, SERVICE, PATH, INTERFACE, METHOD, DONE, CANCELLED };
    State state = TOPIC;
    
    while (state != DONE && state != CANCELLED) {
        switch (state) {
            case TOPIC:
                if (promptMqttTopic(mapping.topic, mapping.topic, true)) {
                    state = SERVICE;
                } else {
                    state = CANCELLED;
                }
                break;
                
            case SERVICE:
                if (promptDbusService(mapping.service, mapping.service, system_bus)) {
                    state = PATH;
                } else {
                    state = TOPIC;
                }
                break;
                
            case PATH:
                if (promptDbusPath(mapping.path, mapping.service, mapping.path, system_bus)) {
                    state = INTERFACE;
                } else {
                    state = SERVICE;
                }
                break;
                
            case INTERFACE:
                if (promptDbusInterface(mapping.interface, mapping.service, mapping.path, mapping.interface, system_bus)) {
                    state = METHOD;
                } else {
                    state = PATH;
                }
                break;
                
            case METHOD:
                if (promptDbusMethod(mapping.method, mapping.service, mapping.path, mapping.interface, mapping.method, system_bus)) {
                    state = DONE;
                } else {
                    state = INTERFACE;
                }
                break;
                
            case DONE:
            case CANCELLED:
                break;
        }
    }
    
    if (state == DONE) {
        std::cout << "\n✓ Mapping updated.\n" << std::endl;
    } else {
        std::cout << "\nEdit cancelled.\n" << std::endl;
    }
}

void ConfigGenerator::deleteDbusToMqttMapping(Config& config, size_t index) {
    const auto& mapping = config.dbus_to_mqtt[index];
    std::cout << "Delete mapping: " << mapping.service << "::" << mapping.signal 
              << " -> " << mapping.topic << std::endl;
    
    if (InteractiveSelector::promptYesNo("Are you sure?", false)) {
        config.dbus_to_mqtt.erase(config.dbus_to_mqtt.begin() + index);
        std::cout << "✓ Mapping deleted.\n" << std::endl;
    }
}

void ConfigGenerator::deleteMqttToDbusMapping(Config& config, size_t index) {
    const auto& mapping = config.mqtt_to_dbus[index];
    std::cout << "Delete mapping: " << mapping.topic 
              << " -> " << mapping.service << "::" << mapping.method << std::endl;
    
    if (InteractiveSelector::promptYesNo("Are you sure?", false)) {
        config.mqtt_to_dbus.erase(config.mqtt_to_dbus.begin() + index);
        std::cout << "✓ Mapping deleted.\n" << std::endl;
    }
}
