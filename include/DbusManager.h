#pragma once

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "Config.h"

class DbusManager {
public:
    using SignalCallback = std::function<void(const DbusToMqttMapping& mapping, const std::vector<sdbus::Variant>& args)>;

    DbusManager(const std::vector<DbusToMqttMapping>& signalMappings, const std::string& busType = "session");
    
    void start();
    void setSignalCallback(SignalCallback cb);
    
    sdbus::Variant callMethod(const std::string& service, 
                             const std::string& path, 
                             const std::string& interface, 
                             const std::string& method,
                             const std::vector<sdbus::Variant>& args);

private:
    std::unique_ptr<sdbus::IConnection> connection_;
    std::vector<std::unique_ptr<sdbus::IProxy>> proxies_;
    SignalCallback signalCallback_;
    std::vector<DbusToMqttMapping> mappings_;
};
