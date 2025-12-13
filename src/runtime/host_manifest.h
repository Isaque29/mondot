#ifndef MONDOT_HOST_MANIFEST_H
#define MONDOT_HOST_MANIFEST_H

#include <unordered_set>
#include <string>
#include <mutex>
#include <shared_mutex>

struct HostManifest
{
    static inline std::unordered_set<std::string> names;
    static inline std::shared_mutex names_mtx;

    static void register_name(const std::string &n)
    {
        std::unique_lock<std::shared_mutex> lock(names_mtx);
        names.insert(n);
    }

    static void unregister_name(const std::string &n)
    {
        std::unique_lock<std::shared_mutex> lock(names_mtx);
        names.erase(n);
    }

    static bool has(const std::string &n)
    {
        std::shared_lock<std::shared_mutex> lock(names_mtx);
        return names.find(n) != names.end();
    }
};

#endif
