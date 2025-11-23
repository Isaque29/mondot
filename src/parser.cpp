#include "parser.h"
#include <stdexcept>

using namespace std;

Parser::Parser(const string &s): lex(s)
{
    cur = lex.next();
}

void Parser::eat()
{
    cur = lex.next();
}

bool Parser::accept(TokenKind k)
{
    if(cur.kind==k)
    {
        eat();
        return true;
    }
    return false;
}

void Parser::expect(TokenKind k, const string &msg)
{
    if(cur.kind!=k)
    {
        throw runtime_error("parse error: expected " + msg + " got '" + cur.text + "' at line " + to_string(cur.line));
    }
    eat();
}

unique_ptr<Program> Parser::parse_program()
{
    auto p=make_unique<Program>();
    while(cur.kind!=TokenKind::End)
    {
        if(cur.kind==TokenKind::Kw_unit) p->units.push_back(parse_unit());
        else throw runtime_error("expected 'unit' at top-level");
    }
    return p;
}

unique_ptr<UnitDecl> Parser::parse_unit()
{
    expect(TokenKind::Kw_unit, "unit");
    if(cur.kind!=TokenKind::Identifier) throw runtime_error("expected unit name");
    
    string uname=cur.text; eat();
    expect(TokenKind::LBrace, "{");
    auto u = make_unique<UnitDecl>();
    u->name = uname;
    while(!(cur.kind==TokenKind::RBrace))
    {
        if(cur.kind==TokenKind::Kw_on) u->handlers.push_back(parse_handler());
        else throw runtime_error("expected 'on' in unit");
    }
    expect(TokenKind::RBrace, "}");
    return u;
}

unique_ptr<HandlerDecl> Parser::parse_handler()
{
    expect(TokenKind::Kw_on, "on");
    if (cur.kind!=TokenKind::Identifier) throw runtime_error("expected event name");
    string hname=cur.text; eat();
    expect(TokenKind::Arrow, "->");
    expect(TokenKind::LParen, "("); // TODO: check params

    while(cur.kind!=TokenKind::RParen && cur.kind!=TokenKind::End)
        eat();
    
    expect(TokenKind::RParen, ")");
    auto h = make_unique<HandlerDecl>();
    h->name = hname;

    while(!(cur.kind==TokenKind::Kw_end))
    {
        if(cur.kind==TokenKind::Semicolon)
        {
            eat();
            continue;
        }
        if(cur.kind==TokenKind::Identifier)
        {
            string id = cur.text; eat();
            if (cur.kind==TokenKind::Equal)
            {
                eat();
                auto e = parse_expression();
                h->body.push_back(Stmt::make_assign(id, move(e)));
                expect(TokenKind::Semicolon, ";");
            }
            else if (cur.kind==TokenKind::LParen)
            {
                auto call = parse_call(id);
                h->body.push_back(Stmt::make_expr(move(call)));
                expect(TokenKind::Semicolon, ";");
            }
            else throw runtime_error("unexpected token after identifier: " + cur.text);
        }
        else if (cur.kind==TokenKind::String || cur.kind==TokenKind::Number)
        {
            auto e = parse_expression();
            h->body.push_back(Stmt::make_expr(move(e)));
            expect(TokenKind::Semicolon, ";");
        }
        else if (cur.kind==TokenKind::End) throw runtime_error("unexpected EOF in handler");
        else eat();
    }
    expect(TokenKind::Kw_end, "end");
    return h;
}

unique_ptr<Expr> Parser::parse_expression()
{
    if(cur.kind==TokenKind::Number)
    {
        double n = stod(cur.text);
        eat();
        return make_unique<Expr>(n);
    }

    if(cur.kind==TokenKind::String)
    {
        string s = cur.text;
        eat();
        return make_unique<Expr>(s, true);
    }
    
    if(cur.kind==TokenKind::Identifier)
    {
        string id = cur.text;
        eat();
        if(cur.kind==TokenKind::LParen)
        {
            return parse_call(id);
        }
        return make_unique<Expr>(id, false);
    }
    throw runtime_error(string("parse expr error at token '") + cur.text + "'"); }

unique_ptr<Expr> Parser::parse_call(const string &name)
{
    expect(TokenKind::LParen, "(");
    vector<unique_ptr<Expr>> args;
    if(cur.kind!=TokenKind::RParen)
    {
        args.push_back(parse_expression());
        while(cur.kind==TokenKind::Comma)
        {
            eat();
            args.push_back(parse_expression());
        }
    }
    expect(TokenKind::RParen, ")");
    return make_unique<Expr>(name, move(args));
}
