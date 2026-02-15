// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#pragma once

#include <string>
#include <vector>
#include <optional>

class InteractiveSelector {
public:
    // Show interactive list with cursor navigation
    // Returns selected item or nullopt if cancelled
    // Special return values: "<<UP>>" for left arrow, "<<SELECT>>" for descend
    static std::optional<std::string> selectFromList(
        const std::string& title,
        const std::vector<std::string>& items,
        bool allow_manual_entry = true,
        bool allow_navigation = false  // Enable left/right for tree navigation
    );
    
    // Show yes/no prompt
    static bool promptYesNo(const std::string& question, bool default_yes = true);
    
    // Show text input with optional default
    // Returns nullopt if user passed .. to go back
    static std::optional<std::string> promptText(
        const std::string& question,
        const std::string& default_value = ""
    );
    
    // Show password input (masked)
    static std::string promptPassword(const std::string& question);
    
private:
    static void initNcurses();
    static void cleanupNcurses();
};
