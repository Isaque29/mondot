#pragma once
#include "value.h"
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <atomic>
#include <shared_mutex>
#include <optional>

using HostFn = std::function<Value(const std::vector<Value>&)>;

struct HostBridge
{
    std::atomic<uint32_t> next_rule_id{1};

    mutable std::shared_mutex fn_mtx;
    std::unordered_map<std::string, HostFn> functions;

    Rule create_rule(const std::string &type);
    void release_rule(const Rule &r);

    void register_function(const std::string &name, HostFn fn);
    bool unregister_function(const std::string &name);
    bool has_function(const std::string &name) const;
    std::optional<Value> call_function(const std::string &name, const std::vector<Value> &args) const;
};

extern HostBridge GLOBAL_HOST;
