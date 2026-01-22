#include "DbusManager.hpp"
#include "TypeUtils.hpp"
#include <iostream>

DbusManager::DbusManager(const std::vector<DbusToMqttMapping>& signalMappings, const std::string& busType)
    : mappings_(signalMappings) {
    connection_ = (busType == "system") ? sdbus::createSystemBusConnection() : sdbus::createSessionBusConnection();
}

void DbusManager::setSignalCallback(SignalCallback cb) {
    signalCallback_ = std::move(cb);
}

void DbusManager::start() {
    for (const auto& mapping : mappings_) {
        auto proxy = sdbus::createProxy(*connection_, mapping.service, mapping.path);
        
        proxy->registerSignalHandler(mapping.interface, mapping.signal, [this, mapping](sdbus::Signal& signal) {
            auto args = TypeUtils::unpackSignal(signal);
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
        else methodCall << arg;
    }
    
    auto reply = proxy->callMethod(methodCall);
    sdbus::Variant result;
    if (reply.isValid()) {
        try {
            reply >> result;
        } catch (...) {}
    }
    return result;
}
