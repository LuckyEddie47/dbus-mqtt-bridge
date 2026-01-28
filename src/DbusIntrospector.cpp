// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#include "DbusIntrospector.h"
#include <sdbus-c++/sdbus-c++.h>
#include <regex>
#include <algorithm>
#include <iostream>

BusServices DbusIntrospector::listAllServices() {
    BusServices services;
    
    try {
        services.system_services = listServices(true);
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not list system bus services: " << e.what() << std::endl;
    }
    
    try {
        services.session_services = listServices(false);
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not list session bus services: " << e.what() << std::endl;
    }
    
    return services;
}

std::vector<std::string> DbusIntrospector::listServices(bool system_bus) {
    auto connection = system_bus ? 
        sdbus::createSystemBusConnection() : 
        sdbus::createSessionBusConnection();
    
    auto proxy = sdbus::createProxy(*connection, "org.freedesktop.DBus", "/org/freedesktop/DBus");
    
    std::vector<std::string> names;
    proxy->callMethod("ListNames")
         .onInterface("org.freedesktop.DBus")
         .storeResultsTo(names);
    
    // Filter out names starting with ':' (unique connection names)
    std::vector<std::string> filtered;
    for (const auto& name : names) {
        if (!name.empty() && name[0] != ':') {
            filtered.push_back(name);
        }
    }
    
    std::sort(filtered.begin(), filtered.end());
    return filtered;
}

IntrospectionData DbusIntrospector::introspect(
    const std::string& service,
    const std::string& path,
    bool system_bus
) {
    std::string xml = callIntrospect(service, path, system_bus);
    return parseIntrospectionXml(xml);
}

std::vector<std::string> DbusIntrospector::getSignalsForInterface(
    const std::string& service,
    const std::string& path,
    const std::string& interface,
    bool system_bus
) {
    std::string xml = callIntrospect(service, path, system_bus);
    
    // Extract signals for specific interface
    std::vector<std::string> signals;
    std::regex interface_regex("<interface name=\"" + interface + "\">(.*?)</interface>", 
                               std::regex::extended);
    std::smatch interface_match;
    
    if (std::regex_search(xml, interface_match, interface_regex)) {
        std::string interface_xml = interface_match[1];
        signals = extractElements(interface_xml, "signal", "name");
    }
    
    return signals;
}

std::vector<std::string> DbusIntrospector::getMethodsForInterface(
    const std::string& service,
    const std::string& path,
    const std::string& interface,
    bool system_bus
) {
    std::string xml = callIntrospect(service, path, system_bus);
    
    // Extract methods for specific interface
    std::vector<std::string> methods;
    std::regex interface_regex("<interface name=\"" + interface + "\">(.*?)</interface>",
                               std::regex::extended);
    std::smatch interface_match;
    
    if (std::regex_search(xml, interface_match, interface_regex)) {
        std::string interface_xml = interface_match[1];
        methods = extractElements(interface_xml, "method", "name");
    }
    
    return methods;
}

bool DbusIntrospector::isSystemBusService(const std::string& service) {
    try {
        auto services = listServices(true);
        return std::find(services.begin(), services.end(), service) != services.end();
    } catch (...) {
        return false;
    }
}

bool DbusIntrospector::isSessionBusService(const std::string& service) {
    try {
        auto services = listServices(false);
        return std::find(services.begin(), services.end(), service) != services.end();
    } catch (...) {
        return false;
    }
}

std::string DbusIntrospector::callIntrospect(
    const std::string& service,
    const std::string& path,
    bool system_bus
) {
    auto connection = system_bus ? 
        sdbus::createSystemBusConnection() : 
        sdbus::createSessionBusConnection();
    
    auto proxy = sdbus::createProxy(*connection, service, path);
    
    std::string xml;
    proxy->callMethod("Introspect")
         .onInterface("org.freedesktop.DBus.Introspectable")
         .storeResultsTo(xml);
    
    return xml;
}

IntrospectionData DbusIntrospector::parseIntrospectionXml(const std::string& xml) {
    IntrospectionData data;
    
    // Extract interfaces
    data.interfaces = extractElements(xml, "interface", "name");
    
    // Extract signals (from all interfaces)
    data.signals = extractElements(xml, "signal", "name");
    
    // Extract methods (from all interfaces)
    data.methods = extractElements(xml, "method", "name");
    
    // Extract child nodes/paths
    data.child_paths = extractElements(xml, "node", "name");
    
    return data;
}

std::vector<std::string> DbusIntrospector::extractElements(
    const std::string& xml,
    const std::string& element_name,
    const std::string& name_attr
) {
    std::vector<std::string> results;
    
    // Pattern: <element_name name="value"
    std::string pattern = "<" + element_name + "\\s+" + name_attr + "=\"([^\"]+)\"";
    std::regex element_regex(pattern);
    
    auto words_begin = std::sregex_iterator(xml.begin(), xml.end(), element_regex);
    auto words_end = std::sregex_iterator();
    
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        results.push_back(match[1]);
    }
    
    // Remove duplicates and sort
    std::sort(results.begin(), results.end());
    results.erase(std::unique(results.begin(), results.end()), results.end());
    
    return results;
}
