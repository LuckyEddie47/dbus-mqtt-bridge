// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#include "Bridge.h"
#include "TypeUtils.h"
#include <iostream>

Bridge::Bridge(const Config& config)
    : config_(config)
{
    dbusManager_ = std::make_unique<DbusManager>(config_.dbus_to_mqtt, config_.bus_type);
    mqttManager_ = std::make_unique<MqttManager>(config_.mqtt, config_.mqtt_to_dbus);
}

void Bridge::start() {
    // Wire up the D-Bus → MQTT signal callback.
    // publish() is safe to call at any time; MqttManager guards against the
    // not-connected case internally and logs a warning if the broker is down.
    dbusManager_->setSignalCallback(
        [this](const DbusToMqttMapping& mapping,
               const std::vector<sdbus::Variant>& args)
        {
            nlohmann::json j = nlohmann::json::array();
            for (const auto& arg : args) {
                j.push_back(TypeUtils::variantToJson(arg));
            }
            mqttManager_->publish(mapping.topic, j.dump());
        });

    // Wire up the MQTT → D-Bus message callback.
    mqttManager_->setMessageCallback(
        [this](const std::string& topic, const std::string& payload) {
            this->onMqttMessage(topic, payload);
        });

    // MqttManager::connect() is now non-blocking: it launches a reconnect
    // thread that attempts the first connection in the background, retrying
    // with exponential backoff if the broker is unavailable.
    mqttManager_->connect();

    // DbusManager::start() registers signal handlers (which do not require the
    // remote services to be present) and enters the D-Bus event loop
    // asynchronously.  A NameOwnerChanged watcher inside DbusManager activates
    // and deactivates per-mapping proxies as services come and go.
    dbusManager_->start();
}

void Bridge::stop() {
    mqttManager_->disconnect();
    // DbusManager's event loop is tied to the connection lifetime and will
    // wind down when the connection object is destroyed (in the destructor).
}

void Bridge::onMqttMessage(const std::string& topic, const std::string& payload) {
    for (const auto& mapping : config_.mqtt_to_dbus) {
        if (mapping.topic == topic) {
            try {
                auto j = nlohmann::json::parse(payload);
                std::vector<sdbus::Variant> args;
                if (j.is_array()) {
                    for (const auto& item : j) {
                        args.push_back(TypeUtils::jsonToVariant(item));
                    }
                } else {
                    args.push_back(TypeUtils::jsonToVariant(j));
                }

                auto result = dbusManager_->callMethod(
                    mapping.service, mapping.path,
                    mapping.interface, mapping.method, args);

                std::cout << "Method call result: "
                          << TypeUtils::variantToJson(result).dump() << std::endl;

            } catch (const std::exception& e) {
                // callMethod throws if the service is currently absent.
                // Log it and carry on — the mapping will work again once the
                // service reappears and NameOwnerChanged reactivates it.
                std::cerr << "Error processing MQTT message for topic "
                          << topic << ": " << e.what() << std::endl;
            }
            break;
        }
    }
}