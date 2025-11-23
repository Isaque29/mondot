#ifndef MONDOT_LEXER_H
#define MONDOT_LEXER_H

#include <string>
#include <cctype>

enum class TokenKind
{
    End, Identifier, Number, String,
    Kw_unit, Kw_on, Kw_end, Arrow,
    LParen, RParen, LBrace, RBrace,
    Equal, Semicolon, Comma
};

struct Token
{
    TokenKind kind;
    std::string text;
    int line, col; 
};

struct Lexer {
    std::string src;
    size_t i=0; int line=1, col=1;
    Lexer() = default;
    explicit Lexer(const std::string &s);
    
    char peek() const;
    char get();
    void skip_ws();
    Token next();
};

#endif
