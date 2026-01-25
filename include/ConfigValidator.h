// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#pragma once

#include <string>
#include <vector>
#include <optional>

struct ValidationError {
    std::string field;
    std::string message;
    std::optional<int> line_number;
    
    ValidationError(const std::string& f, const std::string& m, std::optional<int> line = std::nullopt)
        : field(f), message(m), line_number(line) {}
};

struct ValidationResult {
    bool valid = true;
    std::vector<ValidationError> errors;
    std::vector<std::string> warnings;
    
    void addError(const std::string& field, const std::string& message, std::optional<int> line = std::nullopt) {
        valid = false;
        errors.emplace_back(field, message, line);
    }
    
    void addWarning(const std::string& message) {
        warnings.push_back(message);
    }
    
    bool hasErrors() const { return !valid; }
    bool hasWarnings() const { return !warnings.empty(); }
};

class ConfigValidator {
public:
    // Validate MQTT configuration
    static bool validateMqttBroker(const std::string& broker);
    static bool validateMqttPort(int port);
    static bool validateMqttTopic(const std::string& topic, bool allow_wildcards = false);
    
    // Validate D-Bus configuration
    static bool validateDbusServiceName(const std::string& service);
    static bool validateDbusObjectPath(const std::string& path);
    static bool validateDbusInterfaceName(const std::string& interface);
    static bool validateDbusMemberName(const std::string& member);
    static bool validateBusType(const std::string& bus_type);
    
    // Format validation helpers
    static bool isValidHostname(const std::string& hostname);
    static bool isValidIpAddress(const std::string& ip);
    static bool isValidDnsName(const std::string& name);
    
    // Error message formatters
    static std::string formatValidationErrors(const ValidationResult& result);
    static void printValidationErrors(const ValidationResult& result);
};
