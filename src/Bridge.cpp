#include "Bridge.hpp"
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
            j.push_back(variantToJson(arg));
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
                        args.push_back(jsonToVariant(item));
                    }
                } else {
                    args.push_back(jsonToVariant(j));
                }

                auto result = dbusManager_->callMethod(mapping.service, mapping.path, mapping.interface, mapping.method, args);
                std::cout << "Method call result: " << variantToJson(result).dump() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error processing MQTT message for topic " << topic << ": " << e.what() << std::endl;
            }
            break;
        }
    }
}

nlohmann::json Bridge::variantToJson(const sdbus::Variant& v) {
    if (v.containsValueOfType<std::string>()) return v.get<std::string>();
    if (v.containsValueOfType<int32_t>()) return v.get<int32_t>();
    if (v.containsValueOfType<uint32_t>()) return v.get<uint32_t>();
    if (v.containsValueOfType<int64_t>()) return v.get<int64_t>();
    if (v.containsValueOfType<uint64_t>()) return v.get<uint64_t>();
    if (v.containsValueOfType<bool>()) return v.get<bool>();
    if (v.containsValueOfType<double>()) return v.get<double>();
    if (v.containsValueOfType<int16_t>()) return v.get<int16_t>();
    if (v.containsValueOfType<uint16_t>()) return v.get<uint16_t>();
    if (v.containsValueOfType<uint8_t>()) return v.get<uint8_t>();
    
    return "unsupported type";
}

sdbus::Variant Bridge::jsonToVariant(const nlohmann::json& j) {
    if (j.is_string()) return sdbus::Variant(j.get<std::string>());
    if (j.is_boolean()) return sdbus::Variant(j.get<bool>());
    if (j.is_number_integer()) {
        if (j.get<int64_t>() >= std::numeric_limits<int32_t>::min() && j.get<int64_t>() <= std::numeric_limits<int32_t>::max())
            return sdbus::Variant(static_cast<int32_t>(j.get<int64_t>()));
        return sdbus::Variant(j.get<int64_t>());
    }
    if (j.is_number_unsigned()) {
        if (j.get<uint64_t>() <= std::numeric_limits<uint32_t>::max())
            return sdbus::Variant(static_cast<uint32_t>(j.get<uint64_t>()));
        return sdbus::Variant(j.get<uint64_t>());
    }
    if (j.is_number_float()) return sdbus::Variant(j.get<double>());
    
    return sdbus::Variant("");
}
