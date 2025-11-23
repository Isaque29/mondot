#include "host.h"
#include "util.h"
#include <string>

using namespace std;

Rule HostBridge::create_rule(const string &type)
{
    uint32_t id = next_rule_id.fetch_add(1);
    dbg(string("HostBridge::create_rule type=") + type + " id="+to_string(id));
    return Rule{(uint16_t)1, id};
}

void HostBridge::release_rule(const Rule &r)
{
    dbg(string("HostBridge::release_rule id=") + to_string(r.id));
}

HostBridge GLOBAL_HOST;
