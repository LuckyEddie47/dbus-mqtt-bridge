// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#pragma once

#include <string>

enum class CLIMode {
    RUN_BRIDGE,
    GENERATE_CONFIG,
    HELP,
    VERSION,
    ERROR
};

class CLI {
public:
    // Parse arguments and return mode
    static CLIMode parseArguments(int argc, char** argv);
    
    // Show help message
    static void showHelp(const std::string& program_name);
    
    // Show version
    static void showVersion();
    
    // Show error with help hint
    static void showError(const std::string& message);
};
