#pragma once

#include <mqtt/async_client.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "Config.h"

class MqttManager {
public:
    using MessageCallback = std::function<void(const std::string& topic, const std::string& payload)>;

    MqttManager(const MqttConfig& config, const std::vector<MqttToDbusMapping>& mappings);
    ~MqttManager();

    void connect();
    void disconnect();
    
    void publish(const std::string& topic, const std::string& payload);
    void setMessageCallback(MessageCallback cb);

private:
    class Callback : public virtual mqtt::callback {
        MqttManager& parent_;
    public:
        Callback(MqttManager& parent) : parent_(parent) {}
        void message_arrived(mqtt::const_message_ptr msg) override;
        void connection_lost(const std::string& cause) override;
        void delivery_complete(mqtt::delivery_token_ptr token) override;
    };

    MqttConfig config_;
    std::vector<MqttToDbusMapping> mappings_;
    std::unique_ptr<mqtt::async_client> client_;
    Callback callback_;
    MessageCallback messageCallback_;
};
