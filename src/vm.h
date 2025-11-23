#ifndef MONDOT_VM_H
#define MONDOT_VM_H

#include "module.h"
#include "value.h"
#include "host.h"
#include <string>
#include <vector>

struct Frame {
    Module *module = nullptr;
    ByteFunc *func = nullptr;
    std::vector<Value> locals; // local slots
};

struct VM {
    HostBridge &host;
    VM(HostBridge &h);

    void execute_handler(Module* m, const std::string &handler_name);

    void execute_handler_idx(Module* m, int idx);

    Value load_global(Module* m, const std::string &name);
};

#endif
