#pragma once

#include <sdbus-c++/sdbus-c++.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <iostream>

namespace TypeUtils {

// ── Base64 helpers ────────────────────────────────────────────────────────────
// Used to represent D-Bus blob (ay) as {"_type":"bytes","data":"<base64>"}
// in JSON/MQTT payloads, which round-trips unambiguously in both directions.

inline std::string base64Encode(const std::vector<uint8_t>& data) {
    static constexpr char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    for (size_t i = 0; i < data.size(); i += 3) {
        uint32_t b = static_cast<uint32_t>(data[i]) << 16;
        if (i + 1 < data.size()) b |= static_cast<uint32_t>(data[i + 1]) << 8;
        if (i + 2 < data.size()) b |= static_cast<uint32_t>(data[i + 2]);
        out += kAlphabet[(b >> 18) & 0x3F];
        out += kAlphabet[(b >> 12) & 0x3F];
        out += (i + 1 < data.size()) ? kAlphabet[(b >>  6) & 0x3F] : '=';
        out += (i + 2 < data.size()) ? kAlphabet[(b >>  0) & 0x3F] : '=';
    }
    return out;
}

inline std::vector<uint8_t> base64Decode(const std::string& s) {
    // Lookup table: -1 = ignore (whitespace/unknown), -2 = padding '='
    static constexpr int8_t kDecTable[128] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, //   0-15
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, //  16-31
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63, //  32-47  (+ /)
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-2,-1,-1, //  48-63  (0-9 =)
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14, //  64-79  (A-O)
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1, //  80-95  (P-Z)
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40, //  96-111 (a-o)
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1, // 112-127 (p-z)
    };
    std::vector<uint8_t> out;
    out.reserve((s.size() / 4) * 3);
    uint32_t b = 0;
    int bits = 0;
    for (unsigned char c : s) {
        if (c >= 128) continue;
        int8_t v = kDecTable[c];
        if (v == -1) continue;  // skip whitespace / unknown characters
        if (v == -2) break;     // stop at padding '='
        b = (b << 6) | static_cast<uint32_t>(v);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<uint8_t>((b >> bits) & 0xFF));
        }
    }
    return out;
}

// ── variantToJson ─────────────────────────────────────────────────────────────

inline nlohmann::json variantToJson(const sdbus::Variant& v) {
    if (v.containsValueOfType<std::string>()) return v.get<std::string>();
    if (v.containsValueOfType<bool>())        return v.get<bool>();
    if (v.containsValueOfType<double>())      return v.get<double>();
    if (v.containsValueOfType<int32_t>())     return v.get<int32_t>();
    if (v.containsValueOfType<uint32_t>())    return v.get<uint32_t>();
    if (v.containsValueOfType<int64_t>())     return v.get<int64_t>();
    if (v.containsValueOfType<uint64_t>())    return v.get<uint64_t>();
    if (v.containsValueOfType<int16_t>())     return v.get<int16_t>();
    if (v.containsValueOfType<uint16_t>())    return v.get<uint16_t>();
    if (v.containsValueOfType<uint8_t>())     return v.get<uint8_t>();

    // ay: blob → {"_type":"bytes","data":"<base64>"}
    if (v.containsValueOfType<std::vector<uint8_t>>()) {
        return nlohmann::json{
            {"_type", "bytes"},
            {"data",  base64Encode(v.get<std::vector<uint8_t>>())}
        };
    }

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

// ── jsonToVariant ─────────────────────────────────────────────────────────────

inline sdbus::Variant jsonToVariant(const nlohmann::json& j) {
    if (j.is_string())  return sdbus::Variant(j.get<std::string>());
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

    // Tagged blob: {"_type":"bytes","data":"<base64>"} → ay
    // Must be checked before the generic object handler below.
    if (j.is_object()
        && j.contains("_type") && j["_type"].is_string()
        && j["_type"].get<std::string>() == "bytes"
        && j.contains("data")  && j["data"].is_string())
    {
        return sdbus::Variant(base64Decode(j["data"].get<std::string>()));
    }

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

// ── unpackSignal ──────────────────────────────────────────────────────────────

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

            if      (signature == "s")     { std::string  v; signal >> v; args.emplace_back(v); }
            else if (signature == "i")     { int32_t      v; signal >> v; args.emplace_back(v); }
            else if (signature == "u")     { uint32_t     v; signal >> v; args.emplace_back(v); }
            else if (signature == "x")     { int64_t      v; signal >> v; args.emplace_back(v); }
            else if (signature == "t")     { uint64_t     v; signal >> v; args.emplace_back(v); }
            else if (signature == "b")     { bool         v; signal >> v; args.emplace_back(v); }
            else if (signature == "d")     { double       v; signal >> v; args.emplace_back(v); }
            else if (signature == "y")     { uint8_t      v; signal >> v; args.emplace_back(v); }
            else if (signature == "n")     { int16_t      v; signal >> v; args.emplace_back(v); }
            else if (signature == "q")     { uint16_t     v; signal >> v; args.emplace_back(v); }
            else if (signature == "v")     { sdbus::Variant v; signal >> v; args.push_back(std::move(v)); }
            else if (signature == "as")    { std::vector<std::string> v; signal >> v; args.emplace_back(v); }
            else if (signature == "ai")    { std::vector<int32_t>     v; signal >> v; args.emplace_back(v); }
            // ay: blob
            else if (signature == "ay")    { std::vector<uint8_t>     v; signal >> v; args.emplace_back(v); }
            else if (signature == "a{si}") { std::map<std::string, int32_t>        v; signal >> v; args.emplace_back(v); }
            else if (signature == "a{ss}") { std::map<std::string, std::string>    v; signal >> v; args.emplace_back(v); }
            else if (signature == "a{sv}") { std::map<std::string, sdbus::Variant> v; signal >> v; args.emplace_back(v); }
            else {
                // Unknown type: read it generically as a Variant so the wire
                // iterator advances past it, then continue processing remaining
                // arguments rather than silently truncating the signal.
                std::cerr << "Warning: unpackSignal encountered unsupported D-Bus type '"
                          << signature << "' - inserting as opaque variant" << std::endl;
                sdbus::Variant v;
                signal >> v;
                args.push_back(std::move(v));
            }
        } catch (const std::exception& e) {
            std::cerr << "Error unpacking signal argument of type " << type
                      << ": " << e.what() << std::endl;
            break;
        }
    }

    if (safety_limit <= 0) {
        std::cerr << "Warning: unpackSignal reached safety limit of 100 arguments" << std::endl;
    }

    return args;
}

} // namespace TypeUtils