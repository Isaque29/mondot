#ifndef MONDOT_HOST_H
#define MONDOT_HOST_H

#include "value.h"
#include <atomic>
#include <string>

struct HostBridge {
    std::atomic<uint32_t> next_rule_id{1};
    Rule create_rule(const std::string &type);
    void release_rule(const Rule &r);
};

extern HostBridge GLOBAL_HOST;

#endif
