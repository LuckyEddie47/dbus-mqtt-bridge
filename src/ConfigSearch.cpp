// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#include "ConfigSearch.h"
#include <filesystem>
#include <cstdlib>
#include <iostream>

std::optional<std::string> ConfigSearch::findConfigFile(int argc, char** argv) {
    // 1. Check command line argument (skip if it's a flag)
    if (argc > 1) {
        std::string arg = argv[argc - 1];  // Last argument
        if (arg[0] != '-') {  // Not a flag
            if (fileExists(arg)) {
                return arg;
            } else {
                std::cerr << "Error: Config file not found: " << arg << std::endl;
                return std::nullopt;
            }
        }
    }
    
    // 2. Try user config directory
    std::string userConfig = getUserConfigPath();
    if (!userConfig.empty() && fileExists(userConfig)) {
        return userConfig;
    }
    
    // 3. Try system config directory
    std::string systemConfig = getSystemConfigPath();
    if (fileExists(systemConfig)) {
        return systemConfig;
    }
    
    // 4. Try current directory
    if (fileExists("config.yaml")) {
        return "config.yaml";
    }
    
    // Not found anywhere
    return std::nullopt;
}

std::vector<std::string> ConfigSearch::getSearchPath() {
    std::vector<std::string> paths;
    
    std::string userConfig = getUserConfigPath();
    if (!userConfig.empty()) {
        paths.push_back(userConfig);
    }
    
    paths.push_back(getSystemConfigPath());
    paths.push_back("./config.yaml");
    
    return paths;
}

std::string ConfigSearch::getUserConfigPath() {
    const char* home = std::getenv("HOME");
    if (!home) return "";
    
    return std::string(home) + "/.config/dbus-mqtt-bridge/config.yaml";
}

std::string ConfigSearch::getSystemConfigPath() {
    return "/etc/dbus-mqtt-bridge/config.yaml";
}

bool ConfigSearch::fileExists(const std::string& path) {
    return std::filesystem::exists(path);
}
