#include "DbusManager.hpp"
#include <iostream>

DbusManager::DbusManager(const std::vector<DbusToMqttMapping>& signalMappings, const std::string& busType)
    : mappings_(signalMappings) {
    if (busType == "system") {
        connection_ = sdbus::createSystemBusConnection();
    } else {
        connection_ = sdbus::createSessionBusConnection();
    }
}

void DbusManager::setSignalCallback(SignalCallback cb) {
    signalCallback_ = cb;
}

void DbusManager::start() {
    for (const auto& mapping : mappings_) {
        auto proxy = sdbus::createProxy(*connection_, mapping.service, mapping.path);
        
        proxy->registerSignalHandler(mapping.interface, mapping.signal, [this, mapping](sdbus::Signal& signal) {
            std::vector<sdbus::Variant> args;
            
            while (true) {
                std::string type, contents;
                signal.peekType(type, contents);
                if (type.empty()) break;
                
                try {
                    if (type == "s") {
                        std::string v; signal >> v;
                        args.emplace_back(v);
                    } else if (type == "i") {
                        int32_t v; signal >> v;
                        args.emplace_back(v);
                    } else if (type == "u") {
                        uint32_t v; signal >> v;
                        args.emplace_back(v);
                    } else if (type == "x") {
                        int64_t v; signal >> v;
                        args.emplace_back(v);
                    } else if (type == "t") {
                        uint64_t v; signal >> v;
                        args.emplace_back(v);
                    } else if (type == "b") {
                        bool v; signal >> v;
                        args.emplace_back(v);
                    } else if (type == "d") {
                        double v; signal >> v;
                        args.emplace_back(v);
                    } else if (type == "y") {
                        uint8_t v; signal >> v;
                        args.emplace_back(v);
                    } else if (type == "n") {
                        int16_t v; signal >> v;
                        args.emplace_back(v);
                    } else if (type == "q") {
                        uint16_t v; signal >> v;
                        args.emplace_back(v);
                    } else {
                        std::cout << "[DbusManager] Skipping unknown type: " << type << std::endl;
                        // We need to skip this value somehow or break
                        break; 
                    }
                } catch (const std::exception& e) {
                    std::cerr << "[DbusManager] Error reading signal arg: " << e.what() << std::endl;
                    break;
                }
            }

            if (signalCallback_) {
                signalCallback_(mapping, args);
            }
        });
        
        proxy->finishRegistration();
        proxies_.push_back(std::move(proxy));
    }
    
    connection_->enterEventLoopAsync();
}

sdbus::Variant DbusManager::callMethod(const std::string& service, 
                                     const std::string& path, 
                                     const std::string& interface, 
                                     const std::string& method,
                                     const std::vector<sdbus::Variant>& args) {
    auto proxy = sdbus::createProxy(*connection_, service, path);
    auto methodCall = proxy->createMethodCall(interface, method);
    
    for (const auto& arg : args) {
        if (arg.containsValueOfType<std::string>()) methodCall << arg.get<std::string>();
        else if (arg.containsValueOfType<int32_t>()) methodCall << arg.get<int32_t>();
        else if (arg.containsValueOfType<uint32_t>()) methodCall << arg.get<uint32_t>();
        else if (arg.containsValueOfType<bool>()) methodCall << arg.get<bool>();
        else if (arg.containsValueOfType<double>()) methodCall << arg.get<double>();
        else if (arg.containsValueOfType<int64_t>()) methodCall << arg.get<int64_t>();
        else if (arg.containsValueOfType<uint64_t>()) methodCall << arg.get<uint64_t>();
        else methodCall << arg; // Fallback to sending as Variant
    }
    
    auto reply = proxy->callMethod(methodCall);
    sdbus::Variant result;
    if (reply.isValid()) {
        try {
            reply >> result;
        } catch (...) {
            // Result might be empty or multiple values, for now just return empty variant
        }
    }
    return result;
}
