#pragma once

#include <sdbus-c++/sdbus-c++.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <iostream>

namespace TypeUtils {

inline nlohmann::json variantToJson(const sdbus::Variant& v) {
    if (v.containsValueOfType<std::string>()) return v.get<std::string>();
    if (v.containsValueOfType<bool>()) return v.get<bool>();
    if (v.containsValueOfType<double>()) return v.get<double>();
    if (v.containsValueOfType<int32_t>()) return v.get<int32_t>();
    if (v.containsValueOfType<uint32_t>()) return v.get<uint32_t>();
    if (v.containsValueOfType<int64_t>()) return v.get<int64_t>();
    if (v.containsValueOfType<uint64_t>()) return v.get<uint64_t>();
    if (v.containsValueOfType<int16_t>()) return v.get<int16_t>();
    if (v.containsValueOfType<uint16_t>()) return v.get<uint16_t>();
    if (v.containsValueOfType<uint8_t>()) return v.get<uint8_t>();
    
    if (v.containsValueOfType<std::vector<sdbus::Variant>>()) {
        auto vec = v.get<std::vector<sdbus::Variant>>();
        nlohmann::json j = nlohmann::json::array();
        for (const auto& item : vec) {
            j.push_back(variantToJson(item));
        }
        return j;
    }

    if (v.containsValueOfType<std::vector<std::string>>()) {
        return v.get<std::vector<std::string>>();
    }

    if (v.containsValueOfType<std::vector<int32_t>>()) {
        return v.get<std::vector<int32_t>>();
    }

    if (v.containsValueOfType<std::map<std::string, sdbus::Variant>>()) {
        auto map = v.get<std::map<std::string, sdbus::Variant>>();
        nlohmann::json j = nlohmann::json::object();
        for (const auto& [key, value] : map) {
            j[key] = variantToJson(value);
        }
        return j;
    }

    if (v.containsValueOfType<std::map<std::string, std::string>>()) {
        return v.get<std::map<std::string, std::string>>();
    }

    if (v.containsValueOfType<std::map<std::string, int32_t>>()) {
        return v.get<std::map<std::string, int32_t>>();
    }
    
    return "unsupported type";
}

inline sdbus::Variant jsonToVariant(const nlohmann::json& j) {
    if (j.is_string()) return sdbus::Variant(j.get<std::string>());
    if (j.is_boolean()) return sdbus::Variant(j.get<bool>());
    if (j.is_number_integer()) {
        int64_t val = j.get<int64_t>();
        if (val >= std::numeric_limits<int32_t>::min() && val <= std::numeric_limits<int32_t>::max())
            return sdbus::Variant(static_cast<int32_t>(val));
        return sdbus::Variant(val);
    }
    if (j.is_number_unsigned()) {
        uint64_t val = j.get<uint64_t>();
        if (val <= std::numeric_limits<uint32_t>::max())
            return sdbus::Variant(static_cast<uint32_t>(val));
        return sdbus::Variant(val);
    }
    if (j.is_number_float()) return sdbus::Variant(j.get<double>());
    
    if (j.is_array()) {
        std::vector<sdbus::Variant> vec;
        for (const auto& item : j) {
            vec.push_back(jsonToVariant(item));
        }
        return sdbus::Variant(vec);
    }

    if (j.is_object()) {
        std::map<std::string, sdbus::Variant> map;
        for (auto it = j.begin(); it != j.end(); ++it) {
            map[it.key()] = jsonToVariant(it.value());
        }
        return sdbus::Variant(map);
    }

    return sdbus::Variant("");
}

inline std::vector<sdbus::Variant> unpackSignal(sdbus::Signal& signal) {
    std::vector<sdbus::Variant> args;
    int safety_limit = 100;
    while (safety_limit-- > 0) {
        std::string type, contents;
        signal.peekType(type, contents);
        if (type.empty()) break;
        
        try {
            std::string signature = type;
            if (type == "a" || type == "r" || type == "e") {
                signature = type + contents;
            }

            if (signature == "s") { std::string v; signal >> v; args.emplace_back(v); }
            else if (signature == "i") { int32_t v; signal >> v; args.emplace_back(v); }
            else if (signature == "u") { uint32_t v; signal >> v; args.emplace_back(v); }
            else if (signature == "x") { int64_t v; signal >> v; args.emplace_back(v); }
            else if (signature == "t") { uint64_t v; signal >> v; args.emplace_back(v); }
            else if (signature == "b") { bool v; signal >> v; args.emplace_back(v); }
            else if (signature == "d") { double v; signal >> v; args.emplace_back(v); }
            else if (signature == "y") { uint8_t v; signal >> v; args.emplace_back(v); }
            else if (signature == "n") { int16_t v; signal >> v; args.emplace_back(v); }
            else if (signature == "q") { uint16_t v; signal >> v; args.emplace_back(v); }
            else if (signature == "v") { sdbus::Variant v; signal >> v; args.push_back(std::move(v)); }
            else if (signature == "as") { std::vector<std::string> v; signal >> v; args.emplace_back(v); }
            else if (signature == "ai") { std::vector<int32_t> v; signal >> v; args.emplace_back(v); }
            else if (signature == "a{si}") { std::map<std::string, int32_t> v; signal >> v; args.emplace_back(v); }
            else if (signature == "a{ss}") { std::map<std::string, std::string> v; signal >> v; args.emplace_back(v); }
            else if (signature == "a{sv}") { std::map<std::string, sdbus::Variant> v; signal >> v; args.emplace_back(v); }
            else {
                std::cerr << "Skipping unsupported DBus type: " << type << " (signature: " << signature << ")" << std::endl;
                break;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error unpacking signal argument of type " << type << ": " << e.what() << std::endl;
            break;
        }
    }
    
    if (safety_limit <= 0) {
        std::cerr << "Warning: unpackSignal reached safety limit of 100 arguments" << std::endl;
    }
    
    return args;
}

} // namespace TypeUtils
