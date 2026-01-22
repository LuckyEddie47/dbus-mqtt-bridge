#include <iostream>
#include "Config.hpp"
#include "Bridge.hpp"
#include <thread>
#include <chrono>

int main(int argc, char** argv) {
    std::string configFile = "config.yaml";
    if (argc > 1) {
        configFile = argv[1];
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
