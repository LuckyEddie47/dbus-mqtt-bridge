
#include <iostream>
#include "Config.hpp"
#include "Bridge.hpp"
#include <thread>
#include <chrono>
#include <cstring>
#include "../build/VersionInfo.h"

int main(int argc, char** argv) {

    // CLI argument parsing
    auto print_help = []() {
        std::cout << "dbus-mqtt-bridge - Bi-directional bridge between DBus and MQTT\n"
                  << "\nUsage: dbus-mqtt-bridge [config.yaml]\n"
                  << "\nOptions:\n"
                  << "  -h, --help      Show this help message and exit\n"
                  << "  -v, --version   Show program version and exit\n"
                  << "\nBy default, uses 'config.yaml' in the current directory.\n"
                  << "See the man page (man dbus-mqtt-bridge) for full documentation.\n";
    };

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        }
        if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--version") == 0) {
            std::cout << "dbus-mqtt-bridge version " << CLIENT_VERSION << " (built " << BUILD_TIMESTAMP << ")\n";
            return 0;
        }
    }

    std::string configFile = "config.yaml";
    if (argc > 1) {
        // Skip switches
        for (int i = 1; i < argc; ++i) {
            if (argv[i][0] != '-') {
                configFile = argv[i];
                break;
            }
        }
    }

    try {
        std::cout << "Loading configuration from " << configFile << "..." << std::endl;
        Config config = Config::loadFromFile(configFile);

        std::cout << "Initializing bridge..." << std::endl;
        Bridge bridge(config);

        std::cout << "Starting bridge..." << std::endl;
        bridge.start();

        std::cout << "Bridge is running. Press Ctrl+C to stop." << std::endl;
        
        // Keep main thread alive
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
