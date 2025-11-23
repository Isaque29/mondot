#include "module.h"
#include "util.h"   // dbg/info
#include <utility>

using namespace std;

Module* module_from_compiled(const CompiledUnit &cu)
{
    Module *m = new Module();
    m->name = cu.module.name;
    m->bytecode = cu.module;
    return m;
}

ModuleManager G_MODULES;
atomic_flag super_called = ATOMIC_FLAG_INIT;

Module* ModuleManager::get_module(const string &name)
{
    lock_guard<mutex> lk(modules_mtx);
    auto it = modules.find(name);
    if(it==modules.end()) return nullptr;
    return it->second;
}

void ModuleManager::hot_swap(Module* newm)
{
    string name = newm->name;
    Module* old = nullptr;
    {
        lock_guard<mutex> lk(modules_mtx);
        auto it = modules.find(name);
        if(it==modules.end())
        {
            modules.emplace(name, newm);
        }
        else
        {
            old = it->second;
            it->second = newm;
        }
    }
    if(old)
    {
        lock_guard<mutex> lk(reclaim_mtx);
        pending_reclaim.push_back(old);
        dbg("ModuleManager: queued old module for reclaim: "+old->name);
    }
    dbg("ModuleManager: module " + name + " installed");
}

void ModuleManager::tick_reclaim()
{
    lock_guard<mutex> lk(reclaim_mtx);
    vector<Module*> keep;
    for(Module* m : pending_reclaim)
    {
        if(m->active_calls.load() > 0)
        {
            keep.push_back(m);
        }
        else
        {
            dbg("Reclaiming module " + m->name);
            delete m;
        }
    }
    pending_reclaim.swap(keep);
}
