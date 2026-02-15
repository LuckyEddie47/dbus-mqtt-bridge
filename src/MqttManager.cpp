// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#include "MqttManager.h"
#include <iostream>
#include <chrono>

// ── Backoff parameters ────────────────────────────────────────────────────────
// Retry delay doubles on each failure up to the cap.
static constexpr std::chrono::seconds kInitialRetryDelay{5};
static constexpr std::chrono::seconds kMaxRetryDelay{60};

MqttManager::MqttManager(const MqttConfig& config, const std::vector<MqttToDbusMapping>& mappings)
    : config_(config)
    , mappings_(mappings)
    , callback_(*this)
{
    std::string address = "tcp://" + config_.broker + ":" + std::to_string(config_.port);
    client_ = std::make_unique<mqtt::async_client>(address, "dbus-mqtt-bridge");
    client_->set_callback(callback_);

    // Build connect options once; they are reused on every reconnect attempt.
    if (!config_.username.empty() && !config_.password.empty()) {
        connOpts_.set_user_name(config_.username);
        connOpts_.set_password(config_.password);
    }
    // clean_session=false lets the broker remember our subscriptions across
    // brief disconnections (QoS 1 messages queued during the gap are delivered
    // on reconnect).  We still resubscribe explicitly after every connect to
    // handle the case where the broker was restarted and lost its state.
    connOpts_.set_clean_session(false);
    connOpts_.set_automatic_reconnect(false); // we manage reconnect ourselves
}

MqttManager::~MqttManager() {
    // Signal the reconnect thread to stop before joining.
    {
        std::lock_guard<std::mutex> lock(reconnectMutex_);
        stopReconnect_ = true;
        reconnectNeeded_ = true;
    }
    reconnectCv_.notify_all();

    if (reconnectThread_.joinable()) {
        reconnectThread_.join();
    }

    if (client_ && client_->is_connected()) {
        try {
            client_->disconnect()->wait();
        } catch (...) {}
    }
}

// ── Public API ────────────────────────────────────────────────────────────────

void MqttManager::connect() {
    // Kick off the reconnect thread, which immediately attempts a first
    // connection.  This call returns immediately — the bridge does not block
    // waiting for MQTT to come up.
    reconnectThread_ = std::thread(&MqttManager::reconnectLoop, this);
}

void MqttManager::disconnect() {
    // Stop the reconnect loop first so it cannot race with the disconnect.
    {
        std::lock_guard<std::mutex> lock(reconnectMutex_);
        stopReconnect_ = true;
        reconnectNeeded_ = true;
    }
    reconnectCv_.notify_all();

    if (reconnectThread_.joinable()) {
        reconnectThread_.join();
    }

    try {
        if (client_->is_connected()) {
            client_->disconnect()->wait();
        }
    } catch (const mqtt::exception& exc) {
        std::cerr << "MQTT disconnect error: " << exc.what() << std::endl;
    }
    connected_ = false;
}

void MqttManager::publish(const std::string& topic, const std::string& payload) {
    if (!connected_) {
        // Drop the message and warn.  A future improvement could buffer here.
        std::cerr << "MQTT not connected — dropping message on topic: " << topic << std::endl;
        return;
    }
    try {
        client_->publish(topic, payload, 1, false);
    } catch (const mqtt::exception& exc) {
        std::cerr << "MQTT publish error: " << exc.what() << std::endl;
        // The connection_lost callback will fire shortly and trigger reconnect.
    }
}

void MqttManager::setMessageCallback(MessageCallback cb) {
    messageCallback_ = std::move(cb);
}

// ── Private: reconnect loop ───────────────────────────────────────────────────

void MqttManager::reconnectLoop() {
    auto retryDelay = kInitialRetryDelay;

    while (true) {
        // Wait until either woken (reconnect needed) or stop requested.
        {
            std::unique_lock<std::mutex> lock(reconnectMutex_);
            reconnectCv_.wait(lock, [this] { return reconnectNeeded_ || stopReconnect_.load(); });

            if (stopReconnect_) return;
            reconnectNeeded_ = false;
        }

        // Attempt connection with exponential backoff.
        while (!stopReconnect_) {
            try {
                doConnect();
                retryDelay = kInitialRetryDelay;  // reset on success
                break;
            } catch (const std::exception& e) {
                std::cerr << "MQTT connection failed: " << e.what()
                          << " — retrying in " << retryDelay.count() << "s" << std::endl;
            }

            // Wait for retryDelay, but wake immediately if stop is requested.
            std::unique_lock<std::mutex> lock(reconnectMutex_);
            reconnectCv_.wait_for(lock, retryDelay,
                                  [this] { return stopReconnect_.load(); });

            // Double the delay up to the cap.
            retryDelay = std::min(retryDelay * 2, kMaxRetryDelay);
        }
    }
}

void MqttManager::doConnect() {
    std::cout << "Connecting to MQTT broker at "
              << client_->get_server_uri() << "..." << std::endl;
    client_->connect(connOpts_)->wait();
    std::cout << "MQTT connected." << std::endl;
    connected_ = true;
    resubscribe();
}

void MqttManager::resubscribe() {
    // Called after every successful connect, whether first-time or after
    // reconnect.  Ensures subscriptions are in place even if the broker was
    // restarted and lost its session state.
    for (const auto& mapping : mappings_) {
        std::cout << "Subscribing to MQTT topic: " << mapping.topic << std::endl;
        client_->subscribe(mapping.topic, 1)->wait();
    }
}

// ── Callback inner class ──────────────────────────────────────────────────────

void MqttManager::Callback::connected(const std::string& /*cause*/) {
    // The paho async_client may fire this on automatic reconnect if we ever
    // enable that option.  We do not currently, but handle it defensively.
    std::cout << "MQTT connected (callback)." << std::endl;
    parent_.connected_ = true;
    parent_.resubscribe();
}

void MqttManager::Callback::connection_lost(const std::string& cause) {
    std::cerr << "MQTT connection lost: "
              << (cause.empty() ? "(no reason given)" : cause) << std::endl;
    parent_.connected_ = false;

    // Wake the reconnect loop.
    {
        std::lock_guard<std::mutex> lock(parent_.reconnectMutex_);
        parent_.reconnectNeeded_ = true;
    }
    parent_.reconnectCv_.notify_one();
}

void MqttManager::Callback::message_arrived(mqtt::const_message_ptr msg) {
    if (parent_.messageCallback_) {
        parent_.messageCallback_(msg->get_topic(), msg->to_string());
    }
}

void MqttManager::Callback::delivery_complete(mqtt::delivery_token_ptr /*token*/) {
}