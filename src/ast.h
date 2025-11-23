#ifndef MONDOT_AST_H
#define MONDOT_AST_H

#include <string>
#include <vector>
#include <memory>

struct Expr {
    enum Kind
    {
        KNumber,
        KString,
        KIdent,
        KCall
    } kind;

    double num{};
    std::string str;
    std::string ident;
    std::string call_name;
    std::vector<std::unique_ptr<Expr>> args;

    Expr(double n);
    Expr(const std::string &s, bool isString);
    Expr(const std::string &name, std::vector<std::unique_ptr<Expr>> &&a);
};

struct Stmt {
    enum Kind{KAssign, KExpr} kind;
    std::string lhs; std::unique_ptr<Expr> rhs;
    std::unique_ptr<Expr> expr;
    static std::unique_ptr<Stmt> make_assign(const std::string &l, std::unique_ptr<Expr> r);
    static std::unique_ptr<Stmt> make_expr(std::unique_ptr<Expr> e);
};

struct HandlerDecl
{
    std::string name;
    std::vector<std::string> params;
    std::vector<std::unique_ptr<Stmt>> body;
};

struct UnitDecl
{
    std::string name;
    std::vector<std::unique_ptr<HandlerDecl>> handlers;
};

struct Program
{
    std::vector<std::unique_ptr<UnitDecl>> units;
};

#endif
