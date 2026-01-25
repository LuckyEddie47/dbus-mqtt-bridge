// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#include "ConfigValidator.h"
#include <regex>
#include <iostream>
#include <sstream>

bool ConfigValidator::validateMqttBroker(const std::string& broker) {
    if (broker.empty()) return false;
    
    // Check if it's a valid hostname or IP address
    return isValidHostname(broker) || isValidIpAddress(broker);
}

bool ConfigValidator::validateMqttPort(int port) {
    return port > 0 && port <= 65535;
}

bool ConfigValidator::validateMqttTopic(const std::string& topic, bool allow_wildcards) {
    if (topic.empty()) return false;
    
    // MQTT topic rules:
    // - Can't start with $
    // - Can contain a-z A-Z 0-9 and / - _
    // - Wildcards + and # only allowed in subscriptions
    if (topic[0] == '$') return false;
    
    // Check for wildcards in publish topics
    if (!allow_wildcards && (topic.find('+') != std::string::npos || 
                             topic.find('#') != std::string::npos)) {
        return false;
    }
    
    // If wildcards allowed, # must be at the end and alone
    if (allow_wildcards && topic.find('#') != std::string::npos) {
        if (topic.back() != '#') return false;
        if (topic.length() > 1 && topic[topic.length()-2] != '/') return false;
    }
    
    // Valid characters check
    std::regex valid_topic(R"(^[a-zA-Z0-9/_+#-]+$)");
    return std::regex_match(topic, valid_topic);
}

bool ConfigValidator::validateDbusServiceName(const std::string& service) {
    if (service.empty()) return false;
    
    // D-Bus service name format: org.example.Service
    // Must contain at least one dot
    // Each element must start with letter, contain alphanumeric or underscore
    // No consecutive dots
    if (service.find('.') == std::string::npos) return false;
    if (service.find("..") != std::string::npos) return false;
    
    std::regex service_pattern(R"(^[a-zA-Z_][a-zA-Z0-9_]*(\.[a-zA-Z_][a-zA-Z0-9_]*)+$)");
    return std::regex_match(service, service_pattern);
}

bool ConfigValidator::validateDbusObjectPath(const std::string& path) {
    if (path.empty() || path[0] != '/') return false;
    
    // Root path is valid
    if (path == "/") return true;
    
    // Must not end with /
    if (path.back() == '/') return false;
    
    // No consecutive slashes
    if (path.find("//") != std::string::npos) return false;
    
    // Each path element must contain only [a-zA-Z0-9_]
    std::regex path_pattern(R"(^(/[a-zA-Z0-9_]+)+$)");
    return std::regex_match(path, path_pattern);
}

bool ConfigValidator::validateDbusInterfaceName(const std::string& interface) {
    // Same format as service name
    return validateDbusServiceName(interface);
}

bool ConfigValidator::validateDbusMemberName(const std::string& member) {
    if (member.empty()) return false;
    
    // Must start with letter, contain alphanumeric or underscore
    std::regex member_pattern(R"(^[a-zA-Z_][a-zA-Z0-9_]*$)");
    return std::regex_match(member, member_pattern);
}

bool ConfigValidator::validateBusType(const std::string& bus_type) {
    return bus_type == "system" || bus_type == "session";
}

bool ConfigValidator::isValidHostname(const std::string& hostname) {
    if (hostname.empty() || hostname.length() > 253) return false;
    
    // Hostname can be "localhost" or a domain name
    if (hostname == "localhost") return true;
    
    return isValidDnsName(hostname);
}

bool ConfigValidator::isValidIpAddress(const std::string& ip) {
    // Simple IPv4 validation
    std::regex ipv4_pattern(R"(^(\d{1,3}\.){3}\d{1,3}$)");
    if (!std::regex_match(ip, ipv4_pattern)) return false;
    
    // Check each octet is 0-255
    std::istringstream iss(ip);
    std::string octet;
    while (std::getline(iss, octet, '.')) {
        int val = std::stoi(octet);
        if (val < 0 || val > 255) return false;
    }
    
    return true;
}

bool ConfigValidator::isValidDnsName(const std::string& name) {
    if (name.empty() || name.length() > 253) return false;
    
    // DNS name: labels separated by dots
    // Each label: 1-63 chars, start with letter/digit, contain letter/digit/hyphen
    std::regex dns_pattern(R"(^[a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$)");
    return std::regex_match(name, dns_pattern);
}

std::string ConfigValidator::formatValidationErrors(const ValidationResult& result) {
    std::ostringstream oss;
    
    if (result.hasErrors()) {
        oss << "\nConfiguration validation failed:\n\n";
        
        for (const auto& error : result.errors) {
            if (error.line_number) {
                oss << "  [Line " << *error.line_number << "] ";
            } else {
                oss << "  ";
            }
            
            oss << "Field '" << error.field << "': " << error.message << "\n";
        }
        
        oss << "\nPlease fix these errors and try again.\n";
        oss << "See 'man dbus-mqtt-bridge' for configuration examples.\n";
    }
    
    if (result.hasWarnings()) {
        oss << "\nWarnings:\n";
        for (const auto& warning : result.warnings) {
            oss << "  - " << warning << "\n";
        }
    }
    
    return oss.str();
}

void ConfigValidator::printValidationErrors(const ValidationResult& result) {
    std::string formatted = formatValidationErrors(result);
    if (!formatted.empty()) {
        std::cerr << formatted;
    }
}
