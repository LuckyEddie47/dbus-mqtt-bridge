// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#include "InteractiveSelector.h"
#include <ncurses.h>
#include <algorithm>
#include <iostream>

void InteractiveSelector::initNcurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);  // Hide cursor during selection
}

void InteractiveSelector::cleanupNcurses() {
    curs_set(1);  // Restore cursor
    endwin();
}

std::optional<std::string> InteractiveSelector::selectFromList(
    const std::string& title,
    const std::vector<std::string>& items,
    bool allow_manual_entry,
    bool allow_navigation
) {
    if (items.empty()) {
        return std::nullopt;
    }
    
    initNcurses();
    
    int selected = 0;
    int offset = 0;
    int max_display = LINES - 5;  // Leave room for header and footer
    
    while (true) {
        clear();
        
        // Title
        attron(A_BOLD);
        mvprintw(0, 0, "%s", title.c_str());
        attroff(A_BOLD);
        mvprintw(1, 0, "Use arrow keys to navigate, Enter to select, q to quit");
        if (allow_manual_entry) {
            mvprintw(2, 0, "Press 'm' to enter manually");
        }
        if (allow_navigation) {
            mvprintw(2, allow_manual_entry ? 40 : 0, "Right arrow: descend, Left arrow: go up");
        }
        
        // Calculate visible range
        if (selected < offset) {
            offset = selected;
        }
        if (selected >= offset + max_display) {
            offset = selected - max_display + 1;
        }
        
        int visible_end = std::min(offset + max_display, (int)items.size());
        
        // Show items
        for (int i = offset; i < visible_end; ++i) {
            int y = 4 + (i - offset);
            
            if (i == selected) {
                attron(A_REVERSE);
                mvprintw(y, 2, "> %s", items[i].c_str());
                attroff(A_REVERSE);
            } else {
                mvprintw(y, 2, "  %s", items[i].c_str());
            }
        }
        
        // Show scroll indicators
        if (offset > 0) {
            mvprintw(3, 0, "^ More above");
        }
        if (visible_end < (int)items.size()) {
            mvprintw(4 + max_display, 0, "v More below");
        }
        
        // Footer
        mvprintw(LINES - 1, 0, "Showing %d-%d of %d items",
                 offset + 1, visible_end, (int)items.size());
        
        refresh();
        
        // Handle input
        int ch = getch();
        switch (ch) {
            case KEY_UP:
                if (selected > 0) selected--;
                break;
            case KEY_DOWN:
                if (selected < (int)items.size() - 1) selected++;
                break;
            case KEY_RIGHT:
                if (allow_navigation) {
                    // Signal to descend into selected item
                    cleanupNcurses();
                    return "<<DESCEND>>" + items[selected];
                }
                break;
            case KEY_LEFT:
                if (allow_navigation) {
                    // Signal to go up one level
                    cleanupNcurses();
                    return "<<UP>>";
                }
                break;
            case KEY_PPAGE:  // Page Up
                selected = std::max(0, selected - max_display);
                break;
            case KEY_NPAGE:  // Page Down
                selected = std::min((int)items.size() - 1, selected + max_display);
                break;
            case KEY_HOME:
                selected = 0;
                break;
            case KEY_END:
                selected = items.size() - 1;
                break;
            case 10:  // Enter
            case KEY_ENTER:
                cleanupNcurses();
                return items[selected];
            case 'q':
            case 'Q':
            case 27:  // ESC
                cleanupNcurses();
                return std::nullopt;
            case 'm':
            case 'M':
                if (allow_manual_entry) {
                    cleanupNcurses();
                    std::cout << "Enter manually: ";
                    std::string input;
                    std::getline(std::cin, input);
                    return "<<MANUAL>>" + input;
                }
                break;
        }
    }
}

bool InteractiveSelector::promptYesNo(const std::string& question, bool default_yes) {
    std::cout << question << " [" << (default_yes ? "Y/n" : "y/N") << "]: ";
    std::string input;
    std::getline(std::cin, input);
    
    if (input.empty()) {
        return default_yes;
    }
    
    char first = std::tolower(input[0]);
    return first == 'y';
}

std::optional<std::string> InteractiveSelector::promptText(
    const std::string& question,
    const std::string& default_value
) {
    if (!default_value.empty()) {
        std::cout << question << " [" << default_value << "] (.. to go back): ";
    } else {
        std::cout << question << " (.. to go back): ";
    }
    
    std::string input;
    std::getline(std::cin, input);
    
    // Check if user wants to go back if input is exactly ".." 
    
    if (input == "..") {
        return std::nullopt;
    }
    
    if (input.empty() && !default_value.empty()) {
        return default_value;
    }
    
    return input;
}

std::string InteractiveSelector::promptPassword(const std::string& question) {
    // Use ncurses for password input to hide characters
    initNcurses();
    echo();  // Enable echo but we'll mask it
    curs_set(1);
    
    clear();
    mvprintw(0, 0, "%s: ", question.c_str());
    refresh();
    
    std::string password;
    int ch;
    int x = question.length() + 2;
    int y = 0;
    
    while ((ch = getch()) != '\n' && ch != KEY_ENTER) {
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (!password.empty()) {
                password.pop_back();
                x--;
                mvaddch(y, x, ' ');
                move(y, x);
            }
        } else if (ch >= 32 && ch <= 126) {  // Printable characters
            password += (char)ch;
            mvaddch(y, x, '*');
            x++;
        }
        refresh();
    }
    
    cleanupNcurses();
    return password;
}
