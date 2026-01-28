// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#include "CLI.h"
#include "Version.h"
#include "ConfigSearch.h"
#include <iostream>
#include <string>

CLIMode CLI::parseArguments(int argc, char** argv) {
    // Check for flags
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            return CLIMode::HELP;
        }
        
        if (arg == "-v" || arg == "--version") {
            return CLIMode::VERSION;
        }
        
        if (arg == "--generate-config") {
            return CLIMode::GENERATE_CONFIG;
        }
        
        // Check for unknown flags
        if (arg[0] == '-' && arg != "-o" && arg != "--from") {
            showError("Unknown option: " + arg);
            return CLIMode::ERROR;
        }
    }
    
    return CLIMode::RUN_BRIDGE;
}

void CLI::showHelp(const std::string& program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] [CONFIG_FILE]\n"
              << "\n"
              << "D-Bus to MQTT bridge service\n"
              << "\n"
              << "Options:\n"
              << "  -h, --help            Show this help message\n"
              << "  -v, --version         Show version information\n"
              << "  --generate-config     Interactive configuration generator\n"
              << "                        Use with --from FILE to edit existing config\n"
              << "                        Use with -o FILE to specify output path\n"
              << "\n"
              << "Arguments:\n"
              << "  CONFIG_FILE           Path to configuration file\n"
              << "\n"
              << "If CONFIG_FILE is not specified, searches in order:\n";
    
    auto searchPath = ConfigSearch::getSearchPath();
    for (const auto& path : searchPath) {
        std::cout << "  - " << path << "\n";
    }
    
    std::cout << "\n"
              << "Examples:\n"
              << "  " << program_name << " /etc/dbus-mqtt-bridge/config.yaml\n"
              << "  " << program_name << " --generate-config\n"
              << "  " << program_name << " --generate-config --from config.yaml -o new-config.yaml\n"
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
