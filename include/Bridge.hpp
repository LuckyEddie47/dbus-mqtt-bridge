#pragma once

#include "Config.hpp"
#include "DbusManager.hpp"
#include "MqttManager.hpp"
#include <nlohmann/json.hpp>
#include <memory>
#include <unordered_map>

class Bridge {
public:
    Bridge(const Config& config);
    
    void start();
    void stop();

private:
    void onMqttMessage(const std::string& topic, const std::string& payload);

    Config config_;
    std::unique_ptr<DbusManager> dbusManager_;
    std::unique_ptr<MqttManager> mqttManager_;
    std::unordered_map<std::string, MqttToDbusMapping> mqttMappings_;
};
