// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#include "MqttManager.h"
#include <iostream>

MqttManager::MqttManager(const MqttConfig& config, const std::vector<MqttToDbusMapping>& mappings)
    : config_(config)
    , mappings_(mappings)
    , callback_(*this)
{
    std::string address = "tcp://" + config_.broker + ":" + std::to_string(config_.port);
    client_ = std::make_unique<mqtt::async_client>(address, "dbus-mqtt-bridge");
    client_->set_callback(callback_);
}

MqttManager::~MqttManager() {
    if (client_ && client_->is_connected()) {
        disconnect();
    }
}

void MqttManager::connect() {
    mqtt::connect_options connOpts;
    if (!config_.username.empty() && !config_.password.empty()) {
        connOpts.set_user_name(config_.username);
        connOpts.set_password(config_.password);
    }
    connOpts.set_clean_session(true);

    std::cout << "Connecting to MQTT broker at " << client_->get_server_uri() << "..." << std::endl;
    client_->connect(connOpts)->wait();
    std::cout << "Connected." << std::endl;

    for (const auto& mapping : mappings_) {
        std::cout << "Subscribing to topic: " << mapping.topic << std::endl;
        client_->subscribe(mapping.topic, 1)->wait();
    }
}

void MqttManager::disconnect() {
    try {
        client_->disconnect()->wait();
    } catch (const mqtt::exception& exc) {
        std::cerr << "MQTT Error: " << exc.what() << std::endl;
    }
}

void MqttManager::publish(const std::string& topic, const std::string& payload) {
    try {
        client_->publish(topic, payload, 1, false);
    } catch (const mqtt::exception& exc) {
        std::cerr << "MQTT Error: " << exc.what() << std::endl;
    }
}

void MqttManager::setMessageCallback(MessageCallback cb) {
    messageCallback_ = cb;
}

void MqttManager::Callback::message_arrived(mqtt::const_message_ptr msg) {
    if (parent_.messageCallback_) {
        parent_.messageCallback_(msg->get_topic(), msg->to_string());
    }
}

void MqttManager::Callback::connection_lost(const std::string& cause) {
    std::cerr << "Connection lost: " << cause << std::endl;
}

void MqttManager::Callback::delivery_complete(mqtt::delivery_token_ptr token) {
}
