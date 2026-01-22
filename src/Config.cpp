#include "Config.hpp"
#include <yaml-cpp/yaml.h>
#include <stdexcept>

Config Config::loadFromFile(const std::string& filename) {
    YAML::Node node = YAML::LoadFile(filename);
    Config config;

    if (!node["mqtt"]) throw std::runtime_error("Missing 'mqtt' section in config");
    
    auto mqtt = node["mqtt"];
    config.mqtt.broker = mqtt["broker"].as<std::string>();
    config.mqtt.port = mqtt["port"].as<int>(1883);

    if (node["bus_type"]) {
        config.bus_type = node["bus_type"].as<std::string>();
    }

    if (mqtt["auth"]) {
        auto auth = mqtt["auth"];
        if (auth["username"]) config.mqtt.username = auth["username"].as<std::string>();
        if (auth["password"]) config.mqtt.password = auth["password"].as<std::string>();
    }

    if (node["mappings"]) {
        auto mappings = node["mappings"];
        
        if (mappings["dbus_to_mqtt"]) {
            for (auto m : mappings["dbus_to_mqtt"]) {
                config.dbus_to_mqtt.push_back({
                    m["service"].as<std::string>(),
                    m["path"].as<std::string>(),
                    m["interface"].as<std::string>(),
                    m["signal"].as<std::string>(),
                    m["topic"].as<std::string>()
                });
            }
        }

        if (mappings["mqtt_to_dbus"]) {
            for (auto m : mappings["mqtt_to_dbus"]) {
                config.mqtt_to_dbus.push_back({
                    m["topic"].as<std::string>(),
                    m["service"].as<std::string>(),
                    m["path"].as<std::string>(),
                    m["interface"].as<std::string>(),
                    m["method"].as<std::string>()
                });
            }
        }
    }

    return config;
}
