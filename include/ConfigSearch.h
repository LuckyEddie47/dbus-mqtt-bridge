// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#pragma once

#include <string>
#include <vector>
#include <optional>

class ConfigSearch {
public:
    // Find config file using search path
    // Returns path if found, nullopt if not found
    static std::optional<std::string> findConfigFile(int argc, char** argv);
    
    // Get the search path list (for display in help/errors)
    static std::vector<std::string> getSearchPath();
    
private:
    static std::string getUserConfigPath();
    static std::string getSystemConfigPath();
    static bool fileExists(const std::string& path);
};
