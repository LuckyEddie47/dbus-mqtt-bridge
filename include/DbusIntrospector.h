// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#pragma once

#include <string>
#include <vector>
#include <map>

struct BusServices {
    std::vector<std::string> system_services;
    std::vector<std::string> session_services;
};

struct IntrospectionData {
    std::vector<std::string> signals;
    std::vector<std::string> methods;
    std::vector<std::string> interfaces;
    std::vector<std::string> child_paths;
};

class DbusIntrospector {
public:
    // List services on both buses
    static BusServices listAllServices();
    static std::vector<std::string> listServices(bool system_bus);
    
    // Introspect a service
    static IntrospectionData introspect(
        const std::string& service,
        const std::string& path,
        bool system_bus
    );
    
    // Get specific interface information
    static std::vector<std::string> getSignalsForInterface(
        const std::string& service,
        const std::string& path,
        const std::string& interface,
        bool system_bus
    );
    
    static std::vector<std::string> getMethodsForInterface(
        const std::string& service,
        const std::string& path,
        const std::string& interface,
        bool system_bus
    );
    
    // Determine if a service is on system or session bus
    static bool isSystemBusService(const std::string& service);
    static bool isSessionBusService(const std::string& service);
    
private:
    static std::string callIntrospect(
        const std::string& service,
        const std::string& path,
        bool system_bus
    );
    
    static IntrospectionData parseIntrospectionXml(const std::string& xml);
    static std::vector<std::string> extractElements(
        const std::string& xml,
        const std::string& element_name,
        const std::string& name_attr
    );
};
