// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include "CLI.h"
#include "ConfigSearch.h"
#include "Config.h"
#include "ConfigValidator.h"
#include "Bridge.h"

std::atomic<bool> running{true};

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

int main(int argc, char** argv) {
    // Parse CLI arguments (handles --help, --version)
    int result = CLI::parseArguments(argc, argv);
    if (result != 0) {
        return result == 1 ? 0 : 1;  // 1=success, -1=error
    }
    
    // Find config file
    auto configPath = ConfigSearch::findConfigFile(argc, argv);
    if (!configPath) {
        std::cerr << "Error: No configuration file found." << std::endl;
        std::cerr << "\nSearched locations:" << std::endl;
        for (const auto& path : ConfigSearch::getSearchPath()) {
            std::cerr << "  - " << path << std::endl;
        }
        std::cerr << "\nPlease specify a config file or create one in a default location." << std::endl;
        std::cerr << "See 'dbus-mqtt-bridge --help' for usage." << std::endl;
        return 1;
    }

    try {
        std::cout << "Loading configuration from " << *configPath << "..." << std::endl;
        Config config = Config::loadFromFile(*configPath);

        // Validate configuration
        std::cout << "Validating configuration..." << std::endl;
        ValidationResult validation = config.validate();
        
        if (validation.hasErrors()) {
            ConfigValidator::printValidationErrors(validation);
            return 1;
        }
        
        // Print warnings if any
        if (validation.hasWarnings()) {
            for (const auto& warning : validation.warnings) {
                std::cout << "Warning: " << warning << std::endl;
            }
        }
        
        std::cout << "Configuration valid." << std::endl;

        std::cout << "Initializing bridge..." << std::endl;
        Bridge bridge(config);

        // Set up signal handlers
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        std::cout << "Starting bridge..." << std::endl;
        bridge.start();

        std::cout << "Bridge is running. Press Ctrl+C to stop." << std::endl;
        
        // Keep main thread alive
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        std::cout << "Bridge stopped." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
