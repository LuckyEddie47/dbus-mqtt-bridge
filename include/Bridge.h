#pragma once

#include "Config.h"
#include "DbusManager.h"
#include "MqttManager.h"
#include <nlohmann/json.hpp>
#include <memory>

class Bridge {
public:
    Bridge(const Config& config);

    // Wires up callbacks, launches the MQTT reconnect thread (non-blocking),
    // and starts the D-Bus event loop asynchronously.
    void start();

    // Stops the MQTT reconnect thread and disconnects from the broker.
    // The D-Bus event loop winds down with the connection on destruction.
    void stop();

private:
    void onMqttMessage(const std::string& topic, const std::string& payload);

    Config                       config_;
    std::unique_ptr<DbusManager> dbusManager_;
    std::unique_ptr<MqttManager> mqttManager_;
};