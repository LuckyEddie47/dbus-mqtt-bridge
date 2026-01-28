// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#pragma once

#include "Config.h"
#include <string>
#include <optional>

class ConfigGenerator {
public:
    // Main entry point for config generation
    static int run(int argc, char** argv);
    
private:
    // Parse command line arguments for generator
    static bool parseGeneratorArgs(int argc, char** argv, 
                                   std::string& from_file,
                                   std::string& output_file);
    
    // Load existing/partial config
    static std::optional<Config> loadPartialConfig(const std::string& path);
    
    // Interactive prompts for each section
    static void configureMqtt(Config& config);
    static void configureBusType(Config& config);
    static void configureMappings(Config& config);
    
    // MQTT section prompts
    static void promptMqttBroker(Config& config);
    static void promptMqttPort(Config& config);
    static void promptMqttAuth(Config& config);
    
    // Mapping management
    static void manageDbusToMqttMappings(Config& config);
    static void manageMqttToDbusMapping(Config& config);
    static void addDbusToMqttMapping(Config& config);
    static void addMqttToDbusMapping(Config& config);
    static void editDbusToMqttMapping(Config& config, size_t index);
    static void editMqttToDbusMapping(Config& config, size_t index);
    static void deleteDbusToMqttMapping(Config& config, size_t index);
    static void deleteMqttToDbusMapping(Config& config, size_t index);
    
    // D-Bus field prompts with introspection support (return false to go back)
    static bool promptDbusService(std::string& result, const std::string& current, bool& system_bus);
    static bool promptDbusPath(std::string& result, const std::string& service, 
                               const std::string& current, bool system_bus);
    static bool promptDbusInterface(std::string& result, const std::string& service,
                                    const std::string& path, const std::string& current,
                                    bool system_bus);
    static bool promptDbusSignal(std::string& result, const std::string& service,
                                 const std::string& path, const std::string& interface,
                                 const std::string& current, bool system_bus);
    static bool promptDbusMethod(std::string& result, const std::string& service,
                                 const std::string& path, const std::string& interface,
                                 const std::string& current, bool system_bus);
    static bool promptMqttTopic(std::string& result, const std::string& current, bool for_subscribe);
    
    // Bus type implications
    static void showBusTypeImplications(bool system_bus);
    
    // Output handling
    static bool saveConfig(const Config& config, const std::string& path);
    static std::string configToYaml(const Config& config);
    static void printConfig(const Config& config);
    
    // Validation and error display
    static void showValidationErrors(const Config& config);
    static bool fixValidationErrors(Config& config);
};
