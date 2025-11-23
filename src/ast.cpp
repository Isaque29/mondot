#include "ast.h"

using namespace std;

Expr::Expr(double n): kind(KNumber), num(n){}
Expr::Expr(const string &s, bool isString): kind(isString?KString:KIdent), str(isString? s : string()), ident(isString? string() : s){}
Expr::Expr(const string &name, vector<unique_ptr<Expr>> &&a): kind(KCall), call_name(name), args(move(a)){}

unique_ptr<Stmt> Stmt::make_assign(const string &l, unique_ptr<Expr> r)
{
    auto s=make_unique<Stmt>();
    s->kind=KAssign;
    s->lhs=l;
    s->rhs=move(r);
    return s;
}

unique_ptr<Stmt> Stmt::make_expr(unique_ptr<Expr> e)
{
    auto s=make_unique<Stmt>();
    s->kind=KExpr;
    s->expr=move(e);
    return s;
}
