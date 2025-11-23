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

    // executa handler por nome (procura no bytecode do módulo)
    void execute_handler(Module* m, const std::string &handler_name);

    // executa handler por index (índice em m->bytecode.funcs)
    void execute_handler_idx(Module* m, int idx);

    // carregador de globals (prototipo mínimo)
    Value load_global(Module* m, const std::string &name);
};

#endif // MONDOT_VM_H
