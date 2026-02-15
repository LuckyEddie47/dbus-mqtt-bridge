#pragma once

#include <mqtt/async_client.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Config.h"

class MqttManager {
public:
    using MessageCallback = std::function<void(const std::string& topic, const std::string& payload)>;

    MqttManager(const MqttConfig& config, const std::vector<MqttToDbusMapping>& mappings);
    ~MqttManager();

    // Non-blocking: launches the reconnect thread which attempts the first
    // connection in the background, retrying with exponential backoff.
    void connect();

    // Stops the reconnect thread, then disconnects from the broker.
    void disconnect();

    // Thread-safe: drops the message with a warning if not currently connected.
    void publish(const std::string& topic, const std::string& payload);

    void setMessageCallback(MessageCallback cb);

private:
    // ── paho callback handler ─────────────────────────────────────────────────
    class Callback : public virtual mqtt::callback {
        MqttManager& parent_;
    public:
        explicit Callback(MqttManager& parent) : parent_(parent) {}
        void connected(const std::string& cause) override;
        void connection_lost(const std::string& cause) override;
        void message_arrived(mqtt::const_message_ptr msg) override;
        void delivery_complete(mqtt::delivery_token_ptr token) override;
    };

    // ── reconnect loop helpers ────────────────────────────────────────────────
    void reconnectLoop();
    void doConnect();
    void resubscribe();

    // ── data members ──────────────────────────────────────────────────────────
    MqttConfig                          config_;
    std::vector<MqttToDbusMapping>      mappings_;
    std::unique_ptr<mqtt::async_client> client_;
    Callback                            callback_;
    MessageCallback                     messageCallback_;

    // Built once in the constructor and reused on every reconnect attempt.
    mqtt::connect_options               connOpts_;

    // Set true after a successful connect; cleared by connection_lost.
    // Checked by publish() to avoid calling into a disconnected client.
    std::atomic<bool>                   connected_{false};

    // Reconnect thread state.
    std::thread                         reconnectThread_;
    std::mutex                          reconnectMutex_;
    std::condition_variable             reconnectCv_;
    bool                                reconnectNeeded_{false};  // guarded by reconnectMutex_
    std::atomic<bool>                   stopReconnect_{false};
};