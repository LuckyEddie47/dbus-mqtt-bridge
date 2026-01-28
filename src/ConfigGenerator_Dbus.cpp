// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee
// ConfigGenerator D-Bus prompting with introspection

#include "ConfigGenerator.h"
#include "ConfigValidator.h"
#include "InteractiveSelector.h"
#include "DbusIntrospector.h"
#include <iostream>
#include <algorithm>

bool ConfigGenerator::promptDbusService(std::string& result, const std::string& current, bool& system_bus) {
    while (true) {
        std::cout << "\nEnter D-Bus service name" << std::endl;
        std::cout << "  Press <Return> for empty entry to browse available services" << std::endl;
        std::cout << "  Or enter service name directly" << std::endl;
        
        auto input_opt = InteractiveSelector::promptText("Service", current);
        if (!input_opt) {
            return false;  // User wants to go back
        }
        
        std::string input = *input_opt;
        
        if (input.empty()) {
            // Empty entry - list services from both buses
            std::cout << "\nFetching services from system and session buses..." << std::endl;
            auto services = DbusIntrospector::listAllServices();
            
            // Combine into single list with labels
            std::vector<std::string> all_services;
            all_services.push_back("=== SYSTEM BUS ===");
            for (const auto& svc : services.system_services) {
                all_services.push_back("[SYS] " + svc);
            }
            all_services.push_back("");
            all_services.push_back("=== SESSION BUS ===");
            for (const auto& svc : services.session_services) {
                all_services.push_back("[SES] " + svc);
            }
            
            auto selection = InteractiveSelector::selectFromList(
                "Select D-Bus Service (arrow keys to navigate, Enter to select):",
                all_services,
                true
            );
            
            if (selection) {
                // Extract service name and set bus type
                if (selection->find("[SYS]") != std::string::npos) {
                    input = selection->substr(6);  // Remove "[SYS] "
                    system_bus = true;
                } else if (selection->find("[SES]") != std::string::npos) {
                    input = selection->substr(6);  // Remove "[SES] "
                    system_bus = false;
                } else {
                    continue;  // Header selected, retry
                }
                
                // Show implications of bus choice
                std::cout << "\nSelected: " << input << std::endl;
                showBusTypeImplications(system_bus);
            } else {
                continue;  // Cancelled, retry
            }
        }
        
        // Validate service name
        if (ConfigValidator::validateDbusServiceName(input)) {
            // Check if service exists on the expected bus
            bool on_system = DbusIntrospector::isSystemBusService(input);
            bool on_session = DbusIntrospector::isSessionBusService(input);
            
            if (!on_system && !on_session) {
                std::cout << "⚠  Warning: Service '" << input << "' not found on any bus." << std::endl;
                if (InteractiveSelector::promptYesNo("Continue anyway?", false)) {
                    result = input;
                    return true;
                }
                continue;
            }
            
            if (on_system && system_bus) {
                result = input;
                return true;
            } else if (on_session && !system_bus) {
                result = input;
                return true;
            } else if (on_system && !system_bus) {
                std::cout << "Note: '" << input << "' is a SYSTEM bus service," << std::endl;
                std::cout << "   but session bus is configured." << std::endl;
                if (InteractiveSelector::promptYesNo("Switch to system bus?", true)) {
                    system_bus = true;
                    showBusTypeImplications(true);
                    result = input;
                    return true;
                }
            } else if (on_session && system_bus) {
                std::cout << "Note: '" << input << "' is a SESSION bus service," << std::endl;
                std::cout << "   but system bus is configured." << std::endl;
                if (InteractiveSelector::promptYesNo("Switch to session bus?", true)) {
                    system_bus = false;
                    showBusTypeImplications(false);
                    result = input;
                    return true;
                }
            }
            
            result = input;
            return true;
        } else {
            std::cout << "Invalid service name format. Must be reverse-DNS (e.g., org.example.Service)" << std::endl;
        }
    }
}

bool ConfigGenerator::promptDbusPath(std::string& result, const std::string& service,
                                     const std::string& current,
                                     bool system_bus) {
    std::string current_path = "/";
    
    while (true) {
        std::cout << "\nEnter D-Bus object path" << std::endl;
        std::cout << "  Press <Return> to browse, or enter full path directly" << std::endl;
        
        auto input_opt = InteractiveSelector::promptText("Path", current.empty() ? "" : current);
        if (!input_opt) {
            return false;  // User wants to go back
        }
        
        std::string input = *input_opt;
        
        // If user entered a direct path, validate and return
        if (!input.empty()) {
            if (ConfigValidator::validateDbusObjectPath(input)) {
                result = input;
                return true;
            } else {
                std::cout << "Invalid path. Must start with '/' and contain only [a-zA-Z0-9_/]" << std::endl;
                continue;
            }
        }
        
        // Empty input - enter navigation mode
        while (true) {
            try {
                std::cout << "\nBrowsing at: " << current_path << std::endl;
                std::cout << "Introspecting..." << std::endl;
                auto data = DbusIntrospector::introspect(service, current_path, system_bus);
                
                if (data.child_paths.empty()) {
                    std::cout << "No child paths found at " << current_path << std::endl;
                    if (InteractiveSelector::promptYesNo("Use this path (" + current_path + ")?", true)) {
                        result = current_path;
                        return true;
                    }
                    break; // Exit navigation, return to manual entry
                }
                
                // Build full paths from child names
                std::vector<std::string> full_paths;
                for (const auto& child : data.child_paths) {
                    std::string full_path;
                    if (current_path == "/") {
                        full_path = "/" + child;
                    } else {
                        full_path = current_path + "/" + child;
                    }
                    full_paths.push_back(full_path);
                }
                
                std::string title = "Path: " + current_path + " | Left=up, Right=descend, Enter=select";
                auto selection = InteractiveSelector::selectFromList(
                    title,
                    full_paths,
                    true,
                    true  // Enable navigation mode
                );
                
                if (!selection) {
                    // Cancelled (q pressed) - ask if they want to go back
                    if (InteractiveSelector::promptYesNo("Go back to previous question?", true)) {
                        return false;
                    }
                    break;
                }
                
                // Check for special navigation commands
                if (*selection == "<<UP>>") {
                    // Go up one level
                    size_t pos = current_path.rfind('/');
                    if (pos > 0) {
                        current_path = current_path.substr(0, pos);
                    } else {
                        current_path = "/";
                    }
                    // Continue loop - will refresh display at new level
                    continue;
                } else if (selection->find("<<DESCEND>>") == 0) {
                    // Descend into selected path
                    current_path = selection->substr(11);  // Remove "<<DESCEND>>"
                    // Continue loop - will refresh display at new level
                    continue;
                } else if (selection->find("<<MANUAL>>") == 0) {
                    // User pressed 'm' for manual entry
                    std::string manual = selection->substr(10);  // Remove "<<MANUAL>>"
                    if (!manual.empty() && ConfigValidator::validateDbusObjectPath(manual)) {
                        result = manual;
                        return true;
                    }
                    break; // Return to text prompt
                } else {
                    // Enter pressed - use this path
                    result = *selection;
                    return true;
                }
                
            } catch (const std::exception& e) {
                std::cout << "Error introspecting at " << current_path << ": " << e.what() << std::endl;
                if (InteractiveSelector::promptYesNo("Use current path (" + current_path + ")?", true)) {
                    result = current_path;
                    return true;
                }
                break; // Return to manual entry
            }
        }
    }
}

bool ConfigGenerator::promptDbusInterface(std::string& result, const std::string& service,
                                          const std::string& path,
                                          const std::string& current,
                                          bool system_bus) {
    while (true) {
        std::cout << "\nEnter D-Bus interface name" << std::endl;
        std::cout << "  Press <Return> for empty entry to see available interfaces" << std::endl;
        std::cout << "  Or enter interface directly (e.g., org.example.Interface)" << std::endl;
        
        auto input_opt = InteractiveSelector::promptText("Interface", current);
        if (!input_opt) {
            return false;  // User wants to go back
        }
        
        std::string input = *input_opt;
        
        if (input.empty()) {
            try {
                std::cout << "Introspecting " << service << " at " << path << "..." << std::endl;
                auto data = DbusIntrospector::introspect(service, path, system_bus);
                
                if (!data.interfaces.empty()) {
                    auto selection = InteractiveSelector::selectFromList(
                        "Select interface:",
                        data.interfaces,
                        true,
                        false
                    );
                    if (selection) {
                        if (selection->find("<<MANUAL>>") == 0) {
                            std::string manual = selection->substr(10);
                            if (!manual.empty() && ConfigValidator::validateDbusInterfaceName(manual)) {
                                result = manual;
                                return true;
                            }
                            continue;
                        } else {
                            result = *selection;
                            return true;
                        }
                    } else {
                        if (InteractiveSelector::promptYesNo("Go back to previous question?", true)) {
                            return false;
                        }
                        continue;
                    }
                } else {
                    std::cout << "No interfaces found." << std::endl;
                    continue;
                }
            } catch (const std::exception& e) {
                std::cout << "Error introspecting: " << e.what() << std::endl;
                continue;
            }
        }
        
        if (ConfigValidator::validateDbusInterfaceName(input)) {
            result = input;
            return true;
        } else {
            std::cout << "Invalid interface. Must be reverse-DNS format." << std::endl;
        }
    }
}

bool ConfigGenerator::promptDbusSignal(std::string& result, const std::string& service,
                                       const std::string& path,
                                       const std::string& interface,
                                       const std::string& current,
                                       bool system_bus) {
    while (true) {
        std::cout << "\nEnter D-Bus signal name" << std::endl;
        std::cout << "  Press <Return> for empty entry to see available signals" << std::endl;
        std::cout << "  Or enter signal directly" << std::endl;
        
        auto input_opt = InteractiveSelector::promptText("Signal", current);
        if (!input_opt) {
            return false;  // User wants to go back
        }
        
        std::string input = *input_opt;
        
        if (input.empty()) {
            try {
                std::cout << "Finding signals in " << interface << "..." << std::endl;
                auto signals = DbusIntrospector::getSignalsForInterface(service, path, interface, system_bus);
                
                if (!signals.empty()) {
                    auto selection = InteractiveSelector::selectFromList(
                        "Select signal:",
                        signals,
                        true,
                        false
                    );
                    if (selection) {
                        if (selection->find("<<MANUAL>>") == 0) {
                            std::string manual = selection->substr(10);
                            if (!manual.empty() && ConfigValidator::validateDbusMemberName(manual)) {
                                result = manual;
                                return true;
                            }
                            continue;
                        } else {
                            result = *selection;
                            return true;
                        }
                    } else {
                        if (InteractiveSelector::promptYesNo("Go back to previous question?", true)) {
                            return false;
                        }
                        continue;
                    }
                } else {
                    std::cout << "No signals found in this interface." << std::endl;
                    continue;
                }
            } catch (const std::exception& e) {
                std::cout << "Error introspecting: " << e.what() << std::endl;
                continue;
            }
        }
        
        if (ConfigValidator::validateDbusMemberName(input)) {
            result = input;
            return true;
        } else {
            std::cout << "Invalid signal name. Must start with letter, contain only [a-zA-Z0-9_]" << std::endl;
        }
    }
}

bool ConfigGenerator::promptDbusMethod(std::string& result, const std::string& service,
                                       const std::string& path,
                                       const std::string& interface,
                                       const std::string& current,
                                       bool system_bus) {
    while (true) {
        std::cout << "\nEnter D-Bus method name" << std::endl;
        std::cout << "  Press <Return> for empty entry to see available methods" << std::endl;
        std::cout << "  Or enter method directly" << std::endl;
        
        auto input_opt = InteractiveSelector::promptText("Method", current);
        if (!input_opt) {
            return false;  // User wants to go back
        }
        
        std::string input = *input_opt;
        
        if (input.empty()) {
            try {
                std::cout << "Finding methods in " << interface << "..." << std::endl;
                auto methods = DbusIntrospector::getMethodsForInterface(service, path, interface, system_bus);
                
                if (!methods.empty()) {
                    auto selection = InteractiveSelector::selectFromList(
                        "Select method:",
                        methods,
                        true,
                        false
                    );
                    if (selection) {
                        if (selection->find("<<MANUAL>>") == 0) {
                            std::string manual = selection->substr(10);
                            if (!manual.empty() && ConfigValidator::validateDbusMemberName(manual)) {
                                result = manual;
                                return true;
                            }
                            continue;
                        } else {
                            result = *selection;
                            return true;
                        }
                    } else {
                        if (InteractiveSelector::promptYesNo("Go back to previous question?", true)) {
                            return false;
                        }
                        continue;
                    }
                } else {
                    std::cout << "No methods found in this interface." << std::endl;
                    continue;
                }
            } catch (const std::exception& e) {
                std::cout << "Error introspecting: " << e.what() << std::endl;
                continue;
            }
        }
        
        if (ConfigValidator::validateDbusMemberName(input)) {
            result = input;
            return true;
        } else {
            std::cout << "Invalid method name. Must start with letter, contain only [a-zA-Z0-9_]" << std::endl;
        }
    }
}

bool ConfigGenerator::promptMqttTopic(std::string& result, const std::string& current, bool for_subscribe) {
    while (true) {
        std::cout << "\nEnter MQTT topic" << std::endl;
        if (for_subscribe) {
            std::cout << "  Wildcards allowed: + (single level), # (multi-level)" << std::endl;
        } else {
            std::cout << "  No wildcards allowed for publishing" << std::endl;
        }
        
        auto input_opt = InteractiveSelector::promptText("Topic", current);
        if (!input_opt) {
            return false;  // User wants to go back
        }
        
        std::string input = *input_opt;
        
        if (ConfigValidator::validateMqttTopic(input, for_subscribe)) {
            result = input;
            return true;
        } else {
            if (!for_subscribe && (input.find('+') != std::string::npos || input.find('#') != std::string::npos)) {
                std::cout << "Wildcards not allowed in publish topics." << std::endl;
            } else {
                std::cout << "Invalid topic format." << std::endl;
            }
        }
    }
}

void ConfigGenerator::showBusTypeImplications(bool system_bus) {
    std::cout << "\nBus Type Implications:" << std::endl;
    if (system_bus) {
        std::cout << "   * System bus selected" << std::endl;
        std::cout << "   * Requires root privileges or system service" << std::endl;
        std::cout << "   * Config should be in: /etc/dbus-mqtt-bridge/config.yaml" << std::endl;
        std::cout << "   * Requires D-Bus policy configuration" << std::endl;
        std::cout << "   * Run as: sudo systemctl enable --now dbus-mqtt-bridge" << std::endl;
    } else {
        std::cout << "   * Session bus selected" << std::endl;
        std::cout << "   * Runs as user" << std::endl;
        std::cout << "   * Config can be in: ~/.config/dbus-mqtt-bridge/config.yaml" << std::endl;
        std::cout << "   * No special D-Bus policy needed" << std::endl;
        std::cout << "   * Run as: systemctl --user enable --now dbus-mqtt-bridge" << std::endl;
    }
    std::cout << std::endl;
}
