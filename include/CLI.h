// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#pragma once

#include <string>

class CLI {
public:
    // Parse arguments and handle --help, --version flags
    // Returns: 0 = continue execution, 1 = exit success, -1 = exit error
    static int parseArguments(int argc, char** argv);
    
    // Show help message
    static void showHelp(const std::string& program_name);
    
    // Show version
    static void showVersion();
    
    // Show error with help hint
    static void showError(const std::string& message);
};
