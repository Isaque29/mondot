#ifndef MONDOT_PARSER_H
#define MONDOT_PARSER_H

#include "lexer.h"
#include "ast.h"
#include <memory>

struct Parser {
    Lexer lex;
    Token cur;
    Parser() = delete;
    explicit Parser(const std::string &s);
    void eat();
    bool accept(TokenKind k);
    void expect(TokenKind k, const std::string &msg);

    std::unique_ptr<Program> parse_program();
    std::unique_ptr<UnitDecl> parse_unit();
    std::unique_ptr<HandlerDecl> parse_handler();
    std::unique_ptr<Expr> parse_expression();
    std::unique_ptr<Expr> parse_call(const std::string &name);
};

#endif
