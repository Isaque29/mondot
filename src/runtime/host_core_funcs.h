#pragma once
#include <string>
#include <vector>
#include "value.h"
#include "host.h"

namespace mondot_host
{
    void register_core_host_functions(HostBridge &host);
    void register_extra_host_functions(HostBridge &host);
    struct RegisteredFunctionGuard
    {
        HostBridge *host = nullptr;
        std::string name;

        RegisteredFunctionGuard() = default;
        RegisteredFunctionGuard(HostBridge *h, std::string n);
        ~RegisteredFunctionGuard();

        RegisteredFunctionGuard(const RegisteredFunctionGuard&) = delete;
        RegisteredFunctionGuard& operator=(const RegisteredFunctionGuard&) = delete;

        RegisteredFunctionGuard(RegisteredFunctionGuard&&) noexcept;
        RegisteredFunctionGuard& operator=(RegisteredFunctionGuard&&) noexcept;

        static RegisteredFunctionGuard create(HostBridge *h, const std::string &name,
                                            const HostFn &fn);
    };
}
