#include "bytecode.h"
#include <stdexcept>
#include <unordered_map>
#include <string>

using namespace std;

CompiledUnit compile_unit(UnitDecl *u)
{
    ByteModule mod;
    mod.name = u->name;
    CompiledUnit cu;
    cu.module = mod;
    for(auto &hptr : u->handlers)
    {
        HandlerDecl *h = hptr.get();
        ByteFunc bf;
        unordered_map<string,int> local_index;
        auto add_local = [&](const string &name)->int
        {
            if(local_index.count(name)) return local_index[name];
            int id = (int)bf.locals.size();
            bf.locals.push_back(name);
            local_index[name]=id;
            return id;
        };

        add_local(string("_tmp"));

        for(auto &st : h->body)
        {
            if(st->kind==Stmt::KAssign)
            {
                Expr *e = st->rhs.get();
                if(e->kind==Expr::KNumber)
                {
                    bf.consts.push_back(Value::make_number(e->num));
                    int ci = (int)bf.consts.size()-1;
                    int lid = add_local(st->lhs);
                    bf.code.emplace_back(OP_LOAD_NUM, ci, lid);
                }

                else if(e->kind==Expr::KString)
                {
                    bf.consts.push_back(Value::make_string(e->str));
                    int ci = (int)bf.consts.size()-1;
                    int lid = add_local(st->lhs);
                    bf.code.emplace_back(OP_LOAD_STR, ci, lid);
                }
                else if(e->kind==Expr::KCall)
                {
                    if(e->call_name=="Spawn")
                    {
                        if(e->args.size()>=1 && e->args[0]->kind==Expr::KString)
                        {
                            bf.consts.push_back(Value::make_string(e->args[0]->str));
                            int ci=(int)bf.consts.size()-1;
                            int lid=add_local(st->lhs);
                            bf.code.emplace_back(OP_SPAWN, ci, lid);
                        }
                        else throw runtime_error("Spawn requires string literal in this prototype");
                    }
                    else if(e->call_name=="Print")
                    {
                        throw runtime_error("assignment from Print not supported");
                    }
                    else throw runtime_error(string("unsupported call in assignment: ")+ e->call_name);
                }
                else if(e->kind == Expr::KIdent)
                {
                    int lid = add_local(st->lhs);
                    Op o(OP_LOAD_GLOBAL, 0, lid);
                    o.s = e->ident;
                    bf.code.push_back(o);
                }
                else
                {
                    throw runtime_error("unsupported rhs expr in assignment");
                }
            }
            else if(st->kind == Stmt::KExpr)
            {
                Expr *e = st->expr.get();
                if(e->kind == Expr::KCall)
                {
                    if(e->call_name == "Print")
                    {
                        if(e->args.size() == 1)
                        {
                            Expr *arg = e->args[0].get();
                            if(arg->kind == Expr::KIdent)
                            {
                                Op o(OP_LOAD_GLOBAL, 0, 0);
                                o.s = arg->ident; bf.code.push_back(o);
                                bf.code.emplace_back(OP_PRINT,0,0);
                            }
                            else if(arg->kind == Expr::KString)
                            {
                                bf.consts.push_back(Value::make_string(arg->str));
                                int ci=(int)bf.consts.size()-1;
                                bf.code.emplace_back(OP_LOAD_STR,ci,0);
                                bf.code.emplace_back(OP_PRINT,0,0);
                            }
                            else if(arg->kind == Expr::KNumber)
                            {
                                bf.consts.push_back(Value::make_number(arg->num));
                                int ci=(int)bf.consts.size()-1;
                                bf.code.emplace_back(OP_LOAD_NUM,ci,0);
                                bf.code.emplace_back(OP_PRINT, 0, 0);
                            }
                            else throw runtime_error("unsupported Print arg type");
                        }
                        else throw runtime_error("Print requires 1 arg");
                    }
                    else
                    {
                        if(e->call_name == "Spawn")
                        {
                            if(e->args.size()>=1 && e->args[0]->kind==Expr::KString)
                            {
                                bf.consts.push_back(Value::make_string(e->args[0]->str));
                                int ci=(int)bf.consts.size() - 1;
                                bf.code.emplace_back(OP_SPAWN, ci, -1);
                                bf.code.emplace_back(OP_DROP, -1, 0);
                            }
                            else throw runtime_error("Spawn requires string literal");
                        }
                        else throw runtime_error(string("unsupported call: ")+ e->call_name);
                    }
                }
                else
                {
                    throw runtime_error("unsupported expr stmt");
                }
            }
        }
        for (size_t i=0; i<bf.locals.size(); ++i)
        {
            bf.code.emplace_back(OP_DROP, (int)i, 0);
        }

        bf.code.emplace_back(OP_RET, 0, 0);
        int idx = (int)cu.module.funcs.size();
        cu.module.funcs.push_back(move(bf));
        cu.module.handler_index[h->name]=idx;
    }
    return cu;
}
