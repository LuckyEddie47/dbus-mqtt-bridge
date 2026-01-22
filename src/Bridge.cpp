#include "Bridge.hpp"
#include "TypeUtils.hpp"
#include <iostream>

Bridge::Bridge(const Config& config)
    : config_(config)
{
    dbusManager_ = std::make_unique<DbusManager>(config_.dbus_to_mqtt, config_.bus_type);
    mqttManager_ = std::make_unique<MqttManager>(config_.mqtt, config_.mqtt_to_dbus);
}

void Bridge::start() {
    dbusManager_->setSignalCallback([this](const DbusToMqttMapping& mapping, const std::vector<sdbus::Variant>& args) {
        nlohmann::json j = nlohmann::json::array();
        for (const auto& arg : args) {
            j.push_back(TypeUtils::variantToJson(arg));
        }
        mqttManager_->publish(mapping.topic, j.dump());
    });

    mqttManager_->setMessageCallback([this](const std::string& topic, const std::string& payload) {
        this->onMqttMessage(topic, payload);
    });

    mqttManager_->connect();
    dbusManager_->start();
}

void Bridge::stop() {
    mqttManager_->disconnect();
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

                auto result = dbusManager_->callMethod(mapping.service, mapping.path, mapping.interface, mapping.method, args);
                std::cout << "Method call result: " << TypeUtils::variantToJson(result).dump() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error processing MQTT message for topic " << topic << ": " << e.what() << std::endl;
            }
            break;
        }
    }
}

