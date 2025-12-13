#include "host.h"
#include "host_manifest.h"
#include <mutex>

Rule HostBridge::create_rule(const std::string &type)
{
    uint32_t id = next_rule_id.fetch_add(1);
    return Rule{(uint16_t)1, id};
}
void HostBridge::release_rule(const Rule &r)
{
    // TODO
}

void HostBridge::register_function(const std::string &name, HostFn fn)
{
    {
        std::unique_lock lock(fn_mtx);
        functions[name] = std::move(fn);
    }
    HostManifest::register_name(name);
}

bool HostBridge::unregister_function(const std::string &name)
{
    bool erased = false;
    {
        std::unique_lock lock(fn_mtx);
        erased = functions.erase(name) > 0;
    }
    if(erased) HostManifest::unregister_name(name);
    return erased;
}

bool HostBridge::has_function(const std::string &name) const
{
    std::shared_lock lock(fn_mtx);
    return functions.find(name) != functions.end();
}

std::optional<Value> HostBridge::call_function(const std::string &name, const std::vector<Value> &args) const
{
    std::shared_lock lock(fn_mtx);
    auto it = functions.find(name);
    if(it != functions.end()) {
        HostFn fn = it->second;
        lock.unlock();
        return fn(args);
    }
    return std::nullopt;
}

HostBridge GLOBAL_HOST;
