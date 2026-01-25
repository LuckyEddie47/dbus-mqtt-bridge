// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#include "CLI.h"
#include "Version.h"
#include "ConfigSearch.h"
#include <iostream>
#include <string>

int CLI::parseArguments(int argc, char** argv) {
    // Check for flags
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            showHelp(argv[0]);
            return 1;  // Exit success
        }
        
        if (arg == "-v" || arg == "--version") {
            showVersion();
            return 1;  // Exit success
        }
        
        // Check for unknown flags
        if (arg[0] == '-') {
            showError("Unknown option: " + arg);
            showHelp(argv[0]);
            return -1;  // Exit error
        }
    }
    
    return 0;  // Continue execution
}

void CLI::showHelp(const std::string& program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] [CONFIG_FILE]\n"
              << "\n"
              << "D-Bus to MQTT bridge service\n"
              << "\n"
              << "Options:\n"
              << "  -h, --help       Show this help message\n"
              << "  -v, --version    Show version information\n"
              << "\n"
              << "Arguments:\n"
              << "  CONFIG_FILE      Path to configuration file\n"
              << "\n"
              << "If CONFIG_FILE is not specified, searches in order:\n";
    
    auto searchPath = ConfigSearch::getSearchPath();
    for (const auto& path : searchPath) {
        std::cout << "  - " << path << "\n";
    }
    
    std::cout << "\n"
              << "Examples:\n"
              << "  " << program_name << " /etc/dbus-mqtt-bridge/config.yaml\n"
              << "  " << program_name << " --help\n"
              << "  " << program_name << " --version\n";
}

void CLI::showVersion() {
    std::cout << PROJECT_NAME << " " << PROJECT_VERSION << std::endl;
}

void CLI::showError(const std::string& message) {
    std::cerr << "Error: " << message << std::endl;
    std::cerr << "Try '--help' for more information." << std::endl;
}
