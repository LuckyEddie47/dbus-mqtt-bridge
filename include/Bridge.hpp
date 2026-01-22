#pragma once

#include "Config.hpp"
#include "DbusManager.hpp"
#include "MqttManager.hpp"
#include <nlohmann/json.hpp>
#include <memory>

class Bridge {
public:
    Bridge(const Config& config);
    
    void start();
    void stop();

private:
    void onMqttMessage(const std::string& topic, const std::string& payload);

    nlohmann::json variantToJson(const sdbus::Variant& v);
    sdbus::Variant jsonToVariant(const nlohmann::json& j);

    Config config_;
    std::unique_ptr<DbusManager> dbusManager_;
    std::unique_ptr<MqttManager> mqttManager_;
};
