// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Ed Lee

#include "DbusManager.h"
#include "TypeUtils.h"
#include <iostream>

DbusManager::DbusManager(const std::vector<DbusToMqttMapping>& signalMappings,
                         const std::string& busType)
    : mappings_(signalMappings)
    , busType_(busType)
{
    connection_ = (busType == "system")
        ? sdbus::createSystemBusConnection()
        : sdbus::createSessionBusConnection();
}

void DbusManager::setSignalCallback(SignalCallback cb) {
    signalCallback_ = std::move(cb);
}

// ── start ─────────────────────────────────────────────────────────────────────

void DbusManager::start() {
    // Install a NameOwnerChanged watcher on org.freedesktop.DBus.  This fires
    // whenever any well-known name is acquired or released on the bus, allowing
    // us to activate mappings when a service appears and deactivate them when
    // it disappears.
    watchServiceAppearance();

    // Register signal handlers for every mapping.  In sdbus-c++, creating a
    // proxy and registering a signal handler does NOT require the remote
    // service to be running — the proxy is local.  We therefore attempt
    // registration for every mapping unconditionally at startup.
    //
    // For each mapping we also check right now whether the service is already
    // present on the bus and update activeServices_ accordingly, so that
    // method calls via callMethod() are correctly gated from the start.
    {
        // Query currently active names once.
        std::vector<std::string> currentNames;
        try {
            auto dbusProxy = sdbus::createProxy(
                *connection_, "org.freedesktop.DBus", "/org/freedesktop/DBus");
            dbusProxy->callMethod("ListNames")
                     .onInterface("org.freedesktop.DBus")
                     .storeResultsTo(currentNames);
        } catch (const std::exception& e) {
            std::cerr << "DbusManager: could not list current bus names: "
                      << e.what() << std::endl;
        }

        std::lock_guard<std::mutex> lock(proxiesMutex_);
        for (const auto& name : currentNames) {
            if (!name.empty() && name[0] != ':') {
                activeServices_.insert(name);
            }
        }
    }

    for (const auto& mapping : mappings_) {
        activateMapping(mapping);
    }

    started_ = true;
    connection_->enterEventLoopAsync();
}

// ── watchServiceAppearance ────────────────────────────────────────────────────

void DbusManager::watchServiceAppearance() {
    // Create a proxy to org.freedesktop.DBus itself.  We register a handler
    // for the NameOwnerChanged signal, which carries:
    //   name      — the well-known name that changed
    //   old_owner — unique name of the previous owner (empty if newly appeared)
    //   new_owner — unique name of the new owner    (empty if just disappeared)
    busProxy_ = sdbus::createProxy(
        *connection_, "org.freedesktop.DBus", "/org/freedesktop/DBus");

    busProxy_->registerSignalHandler(
        "org.freedesktop.DBus",
        "NameOwnerChanged",
        [this](sdbus::Signal& signal) {
            std::string name, old_owner, new_owner;
            signal >> name >> old_owner >> new_owner;
            onNameOwnerChanged(name, old_owner, new_owner);
        });

    busProxy_->finishRegistration();
}

// ── onNameOwnerChanged ────────────────────────────────────────────────────────

void DbusManager::onNameOwnerChanged(const std::string& name,
                                     const std::string& old_owner,
                                     const std::string& new_owner) {
    // Ignore unique names (":1.123") — we only care about well-known names.
    if (name.empty() || name[0] == ':') return;

    const bool appeared   = old_owner.empty() && !new_owner.empty();
    const bool disappeared = !old_owner.empty() && new_owner.empty();

    if (appeared) {
        std::cout << "DbusManager: service appeared: " << name << std::endl;

        // Mark the service active and activate any of its mappings whose
        // proxies were registered but not yet live.
        {
            std::lock_guard<std::mutex> lock(proxiesMutex_);
            activeServices_.insert(name);
        }

        // Re-register the signal handlers for any mapping on this service.
        // The existing proxy object is still valid; calling finishRegistration
        // again on a fresh proxy re-establishes the match rule with the daemon.
        for (const auto& mapping : mappings_) {
            if (mapping.service == name) {
                std::cout << "DbusManager: activating mapping "
                          << mapping.service << " " << mapping.path
                          << " " << mapping.signal << std::endl;
                activateMapping(mapping);
            }
        }
    } else if (disappeared) {
        std::cout << "DbusManager: service disappeared: " << name << std::endl;

        std::lock_guard<std::mutex> lock(proxiesMutex_);
        activeServices_.erase(name);
        // The proxy objects remain in proxies_ — their signal handlers will
        // simply not fire while the service is absent, and they will start
        // receiving signals again automatically once the service reappears
        // and we call activateMapping for them above.
    }
}

// ── activateMapping ───────────────────────────────────────────────────────────

void DbusManager::activateMapping(const DbusToMqttMapping& mapping) {
    try {
        auto proxy = sdbus::createProxy(*connection_, mapping.service, mapping.path);

        proxy->registerSignalHandler(
            mapping.interface,
            mapping.signal,
            [this, mapping](sdbus::Signal& signal) {
                auto args = TypeUtils::unpackSignal(signal);
                if (signalCallback_) {
                    signalCallback_(mapping, args);
                }
            });

        proxy->finishRegistration();

        std::lock_guard<std::mutex> lock(proxiesMutex_);
        proxies_.push_back(std::move(proxy));

    } catch (const std::exception& e) {
        // Log the failure but do not propagate — the NameOwnerChanged handler
        // will retry when the service appears.
        std::cerr << "DbusManager: failed to register signal handler for "
                  << mapping.service << " " << mapping.signal
                  << ": " << e.what()
                  << " (will retry when service appears)" << std::endl;
    }
}

// ── callMethod ────────────────────────────────────────────────────────────────

sdbus::Variant DbusManager::callMethod(const std::string& service,
                                       const std::string& path,
                                       const std::string& interface,
                                       const std::string& method,
                                       const std::vector<sdbus::Variant>& args) {
    // Gate on whether the target service is currently known to be active.
    {
        std::lock_guard<std::mutex> lock(proxiesMutex_);
        if (activeServices_.find(service) == activeServices_.end()) {
            throw std::runtime_error(
                "D-Bus service '" + service + "' is not currently available");
        }
    }

    auto proxy = sdbus::createProxy(*connection_, service, path);
    auto methodCall = proxy->createMethodCall(interface, method);

    for (const auto& arg : args) {
        if      (arg.containsValueOfType<std::string>())  methodCall << arg.get<std::string>();
        else if (arg.containsValueOfType<int32_t>())      methodCall << arg.get<int32_t>();
        else if (arg.containsValueOfType<uint32_t>())     methodCall << arg.get<uint32_t>();
        else if (arg.containsValueOfType<bool>())         methodCall << arg.get<bool>();
        else if (arg.containsValueOfType<double>())       methodCall << arg.get<double>();
        else if (arg.containsValueOfType<int64_t>())      methodCall << arg.get<int64_t>();
        else if (arg.containsValueOfType<uint64_t>())     methodCall << arg.get<uint64_t>();
        else                                               methodCall << arg;
    }

    auto reply = proxy->callMethod(methodCall);
    sdbus::Variant result;
    if (reply.isValid()) {
        try {
            reply >> result;
        } catch (...) {}
    }
    return result;
}