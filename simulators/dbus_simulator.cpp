#include <sdbus-c++/sdbus-c++.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

int main() {
    const char* serviceName = "com.zencoder.simulator";
    const char* objectPath = "/com/zencoder/simulator";
    const char* interfaceName = "com.zencoder.simulator";

    auto connection = sdbus::createSessionBusConnection();
    connection->requestName(serviceName);
    auto simulator = sdbus::createObject(*connection, objectPath);

    simulator->registerMethod("echo").onInterface(interfaceName).implementedAs([](const std::string& input) {
        std::cout << "[DBus Sim] Method 'echo' called with: " << input << std::endl;
        return "Echo: " + input;
    });

    simulator->registerSignal("notify").onInterface(interfaceName).withParameters<std::string, int32_t>();
    simulator->registerSignal("complex_signal").onInterface(interfaceName).withParameters<std::vector<std::string>, std::map<std::string, int32_t>>();

    simulator->finishRegistration();

    std::cout << "[DBus Sim] Service running on Session Bus..." << std::endl;
    std::cout << "[DBus Sim] Service: " << serviceName << std::endl;
    std::cout << "[DBus Sim] Path: " << objectPath << std::endl;

    // Run event loop in a separate thread
    connection->enterEventLoopAsync();

    int count = 0;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::string msg = "Periodic notification " + std::to_string(++count);
        std::cout << "[DBus Sim] Emitting signal 'notify' with: " << msg << ", " << count << std::endl;
        simulator->emitSignal("notify").onInterface(interfaceName).withArguments(msg, count);

        std::vector<std::string> arr = {"apple", "banana", "cherry"};
        std::map<std::string, int32_t> dict = {{"x", 10}, {"y", 20}};
        std::cout << "[DBus Sim] Emitting 'complex_signal'" << std::endl;
        simulator->emitSignal("complex_signal").onInterface(interfaceName).withArguments(arr, dict);
    }

    return 0;
}
