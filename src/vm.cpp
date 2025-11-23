#include "vm.h"
#include "util.h"
#include <iostream>

using namespace std;

VM::VM(HostBridge &h): host(h) {}

void VM::execute_handler(Module* m, const string &handler_name)
{
    if(!m) return;
    auto it = m->bytecode.handler_index.find(handler_name);
    if(it==m->bytecode.handler_index.end()){
        dbg("handler not found: " + handler_name + " in module " + m->name);
        return;
    }
    int idx = it->second;
    execute_handler_idx(m, idx);
}

void VM::execute_handler_idx(Module* m, int idx)
{
    if(!m) return;

    if(idx < 0 || idx >= (int)m->bytecode.funcs.size())
    {
        dbg("execute_handler_idx: invalid idx");
        return;
    }

    ByteFunc &f = m->bytecode.funcs[idx];
    Frame frame;
    frame.module = m;
    frame.func = &f;
    frame.locals.resize(f.locals.size());

    m->active_calls.fetch_add(1);
    dbg(string("VM: enter handler ") + to_string(idx) + " in module " + m->name);

    for(size_t ip = 0; ip < f.code.size(); ++ip)
    {
        Op &op = f.code[ip];
        switch(op.op){
            case OP_LOAD_NUM:
            {
                Value v = f.consts[op.a]; // copy
                if(op.b >= 0 && op.b < (int)frame.locals.size()) frame.locals[op.b] = v;
                break;
            }
            case OP_LOAD_STR:
            {
                Value v = f.consts[op.a];
                if(op.b >= 0 && op.b < (int)frame.locals.size()) frame.locals[op.b] = v;
                break;
            }
            case OP_LOAD_GLOBAL:
            {
                string gname = op.s;
                Value gv = load_global(m, gname);
                if(op.b >= 0 && op.b < (int)frame.locals.size()) frame.locals[op.b] = gv;
                break;
            }
            case OP_STORE_GLOBAL:
            {
                // TODO
                break;
            }
            case OP_PRINT:
            {
                bool printed = false;
                for(int i = (int)frame.locals.size() - 1; i >= 0; --i)
                {
                    if(frame.locals[i].tag != Tag::Nil)
                    {
                        cout << value_to_string(frame.locals[i]) << '\n';
                        cout.flush();
                        printed = true;
                        break;
                    }
                }
                if(!printed) cout << "nil" << endl;
                break;
            }
            case OP_SPAWN:
            {
                Value arg = f.consts[op.a];
                string typ = (arg.tag == Tag::String && arg.s) ? *arg.s : string("?");
                Rule r = host.create_rule(typ);
                Value rv = Value::make_rule(r);
                if(op.b >= 0 && op.b < (int)frame.locals.size()) frame.locals[op.b] = rv;
                else dbg("spawn returned tmp dropped");
                break;
            }
            case OP_DROP:
            {
                int slot = op.a;
                if(slot == -1)
                {
                    if(!frame.locals.empty())
                    {
                        int last = (int)frame.locals.size() - 1;
                        frame.locals[last] = Value();
                    }
                }
                else if(slot >= 0 && slot < (int)frame.locals.size())
                    frame.locals[slot] = Value();
                break;
            }
            case OP_RET:
            {
                for(auto &lv : frame.locals) lv = Value();
                
                m->active_calls.fetch_sub(1);
                dbg("VM: exit handler");
                return;
            }
            default:
                dbg("unknown opcode");
                break;
        }
    }

    // if the code has reached its end without RET.
    for(auto &lv : frame.locals)
        lv = Value();
    
    m->active_calls.fetch_sub(1);
}

Value VM::load_global(Module* m, const string &name)
{
    // prototype: no globals table -> returns nil and logs
    dbg("load_global: " + name + " (not implemented) in module " + m->name);
    return Value::make_nil();
}
