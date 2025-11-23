#ifndef MONDOT_BYTECODE_H
#define MONDOT_BYTECODE_H

#include "value.h"
#include <string>
#include <vector>
#include <unordered_map>

enum OpCode : uint8_t
{
    OP_NOP=0,
    OP_LOAD_NUM,
    OP_LOAD_STR,
    OP_LOAD_GLOBAL,
    OP_STORE_GLOBAL,
    OP_PRINT,
    OP_SPAWN,
    OP_RET,
    OP_DROP
};

struct Op
{
    OpCode op;
    int a;
    int b;
    std::string s;
    Op(OpCode o=OP_NOP,int A=0,int B=0): op(o), a(A), b(B){}
};

struct ByteFunc
{
    std::vector<Op> code;
    std::vector<Value> consts;
    std::vector<std::string> locals;
};
struct ByteModule
{
    std::string name;
    std::unordered_map<std::string,int> handler_index;
    std::vector<ByteFunc> funcs;
};

struct CompiledUnit
{
    ByteModule module;
};

#include "ast.h"

CompiledUnit compile_unit(UnitDecl *u);

#endif
