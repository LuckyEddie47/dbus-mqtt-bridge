#pragma once

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <mutex>
#include <set>
#include <stdexcept>
#include "Config.h"

class DbusManager {
public:
    using SignalCallback = std::function<void(const DbusToMqttMapping& mapping,
                                             const std::vector<sdbus::Variant>& args)>;

    DbusManager(const std::vector<DbusToMqttMapping>& signalMappings,
                const std::string& busType = "session");

    // Registers NameOwnerChanged watcher, performs initial service scan,
    // activates all mappings, and enters the D-Bus event loop asynchronously.
    // Does not throw if individual services are absent at startup.
    void start();

    void setSignalCallback(SignalCallback cb);

    // Throws std::runtime_error if the target service is not currently active,
    // so callers can handle the absence gracefully rather than getting an
    // opaque sdbus exception.
    sdbus::Variant callMethod(const std::string& service,
                              const std::string& path,
                              const std::string& interface,
                              const std::string& method,
                              const std::vector<sdbus::Variant>& args);

private:
    // ── NameOwnerChanged handling ─────────────────────────────────────────────

    // Installs the NameOwnerChanged signal handler on org.freedesktop.DBus.
    void watchServiceAppearance();

    // Called from the NameOwnerChanged handler to update activeServices_ and
    // re-register proxies when a watched service appears or disappears.
    void onNameOwnerChanged(const std::string& name,
                            const std::string& old_owner,
                            const std::string& new_owner);

    // Creates a fresh proxy for a mapping and registers its signal handler.
    // Catches and logs any sdbus exception so start() does not abort on a
    // missing service.
    void activateMapping(const DbusToMqttMapping& mapping);

    // ── data members ──────────────────────────────────────────────────────────
    std::string                                      busType_;
    std::unique_ptr<sdbus::IConnection>              connection_;

    // Proxy to org.freedesktop.DBus, held alive for the NameOwnerChanged watch.
    std::unique_ptr<sdbus::IProxy>                   busProxy_;

    // Signal-handler proxies for each activated mapping.
    // Guarded by proxiesMutex_ because activateMapping() can be called from
    // the D-Bus event thread (via onNameOwnerChanged) after start().
    std::vector<std::unique_ptr<sdbus::IProxy>>      proxies_;
    std::mutex                                       proxiesMutex_;

    // Well-known names currently active on the bus.
    // Guarded by proxiesMutex_ (same lock as proxies_ for simplicity).
    std::set<std::string>                            activeServices_;

    SignalCallback                                   signalCallback_;
    std::vector<DbusToMqttMapping>                   mappings_;

    // Set to true after enterEventLoopAsync(); used to distinguish the initial
    // startup phase from callbacks fired later by the event loop.
    std::atomic<bool>                                started_{false};
};