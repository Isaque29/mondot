#include "parser.h"
#include "facts.h"
#include <stdexcept>
#include <sstream>

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
    if(cur.kind!=k) throw runtime_error(string("parse error: expected ")+msg+" got '"+cur.text+"' at line "+to_string(cur.line));
    eat();
}

unique_ptr<Program> Parser::parse_program()
{
    auto p = make_unique<Program>();
    while(cur.kind != TokenKind::End)
    {
        if(cur.kind == TokenKind::Kw_unit) p->units.push_back(parse_unit());
        else throw runtime_error("expected 'unit' at top-level");
    }
    return p;
}

unique_ptr<UnitDecl> Parser::parse_unit()
{
    expect(TokenKind::Kw_unit, "unit");
    if(cur.kind != TokenKind::Identifier) throw runtime_error("expected unit name");

    string uname = cur.text;
    eat();
    expect(TokenKind::LBrace, "{");
    auto u = make_unique<UnitDecl>();
    u->name = uname;

    while(cur.kind != TokenKind::RBrace)
    {
        if(cur.kind == TokenKind::Kw_on) u->handlers.push_back(parse_handler());
        else u->handlers.push_back(parse_annon_handler());
    }
    expect(TokenKind::RBrace, "}");
    return u;
}

unique_ptr<HandlerDecl> Parser::parse_annon_handler()
{
    auto h = make_unique<HandlerDecl>();
    h->name = "MdSuperInit";
    h->params = vector<string>();

    while(cur.kind != TokenKind::Kw_on && cur.kind != TokenKind::RBrace && cur.kind != TokenKind::End)
    {
        if(cur.kind == TokenKind::Semicolon)
        {
            eat();
            continue;
        }

        h->body.push_back(parse_statement());
    }
    return h;
}

unique_ptr<HandlerDecl> Parser::parse_handler()
{
    expect(TokenKind::Kw_on, "on");
    if(cur.kind != TokenKind::Identifier) throw runtime_error("expected function name");

    string hname = cur.text;
    eat();

    if(cur.kind == TokenKind::Arrow || cur.kind == TokenKind::Equal)
        expect(TokenKind::Arrow, "->");
    else if(cur.kind == TokenKind::Equal)
        expect(TokenKind::Equal, "=");
    else throw runtime_error("expected a function or at least an event id.");
    
    expect(TokenKind::LParen, "(");

    // TODO: better handling
    vector<string> params;
    if(cur.kind != TokenKind::RParen)
    {
        if(cur.kind == TokenKind::Identifier)
        {
            params.push_back(cur.text);
            eat();
            while(cur.kind == TokenKind::Comma)
            {
                eat();
                if(cur.kind==TokenKind::Identifier)
                {
                    params.push_back(cur.text);
                    eat();
                }
                else throw runtime_error("expected param name");
            }
        }
    }
    expect(TokenKind::RParen, ")");

    auto h = make_unique<HandlerDecl>();
    h->name = hname;
    h->params = params;

    while(cur.kind != TokenKind::Kw_end)
    {
        if(cur.kind == TokenKind::Semicolon)
        {
            eat();
            continue;
        }

        h->body.push_back(parse_statement());
    }
    expect(TokenKind::Kw_end, "end");
    return h;
}

unique_ptr<Stmt> Parser::parse_statement()
{
    if(cur.kind == TokenKind::Kw_local)
    {
        // local declaration: local id = expr ;
        eat();
        if(cur.kind != TokenKind::Identifier) throw runtime_error("expected identifier after local");
        string name = cur.text; eat();
        if(cur.kind == TokenKind::Equal)
        {
            eat();
            auto init = parse_expression();
            expect(TokenKind::Semicolon, ";");
            return Stmt::make_local(name, move(init));
        }
        else
        {
            expect(TokenKind::Semicolon, ";");
            return Stmt::make_local(name, nullptr);
        }
    }
    if(cur.kind == TokenKind::Kw_if)
    {
        // if (expr) stmts [elseif (expr) stmts]* [else stmts] end
        eat();
        expect(TokenKind::LParen, "(");
        auto cond = parse_expression();
        expect(TokenKind::RParen, ")");
        vector<unique_ptr<Stmt>> then_body;

        while(!(cur.kind==TokenKind::Kw_elseif||cur.kind==TokenKind::Kw_else||cur.kind==TokenKind::Kw_end))
            then_body.push_back(parse_statement());

            auto s = Stmt::make_if(move(cond), move(then_body));
        // elseif parts
        while(cur.kind == TokenKind::Kw_elseif)
        {
            eat();
            expect(TokenKind::LParen, "(");
            auto econd = parse_expression();
            expect(TokenKind::RParen, ")");
            vector<unique_ptr<Stmt>> eb;

            while(!(cur.kind==TokenKind::Kw_elseif||cur.kind==TokenKind::Kw_else||cur.kind==TokenKind::Kw_end))
                eb.push_back(parse_statement());

            s->elseif_parts.emplace_back(move(econd), move(eb));
        }
        if(cur.kind == TokenKind::Kw_else)
        {
            eat();
            vector<unique_ptr<Stmt>> elseb;

            while(!(cur.kind==TokenKind::Kw_end))
                elseb.push_back(parse_statement());
            s->else_body = move(elseb);
        }
        expect(TokenKind::Kw_end, "end");
        return s;
    }
    if(cur.kind == TokenKind::Kw_while)
    {
        eat();
        expect(TokenKind::LParen, "(");
        auto cond = parse_expression();
        expect(TokenKind::RParen, ")");

        vector<unique_ptr<Stmt>> body;
        while(cur.kind != TokenKind::Kw_end)
            body.push_back(parse_statement());
        
        expect(TokenKind::Kw_end, "end");
        return Stmt::make_while(move(cond), move(body));
    }
    if(cur.kind == TokenKind::Kw_foreach)
    {
        eat();
        expect(TokenKind::LParen, "(");
        if(cur.kind != TokenKind::Identifier) throw runtime_error("expected identifier after foreach");

        string itname = cur.text; eat();
        expect(TokenKind::Kw_in, "in");
        auto iter_expr = parse_expression();
        expect(TokenKind::RParen, ")");

        vector<unique_ptr<Stmt>> body;
        while(cur.kind != TokenKind::Kw_end)
            body.push_back(parse_statement());

        expect(TokenKind::Kw_end, "end");
        return Stmt::make_foreach(itname, move(iter_expr), move(body));
    }
    if(cur.kind == TokenKind::Kw_return)
    {
        eat();
        auto e = parse_expression();
        expect(TokenKind::Semicolon, ";");
        return Stmt::make_return(move(e));
    }

    // local assign / identifier start (could be assignment or call)
    if(cur.kind == TokenKind::Identifier)
    {
        string id = cur.text;
        eat();
        if(cur.kind == TokenKind::Equal)
        {
            // assignment
            eat();
            auto rhs = parse_expression();
            expect(TokenKind::Semicolon, ";");
            return Stmt::make_assign(id, move(rhs));
        }
        else if(cur.kind == TokenKind::LParen)
        {
            // call expression
            auto call = parse_call_expr(id);
            expect(TokenKind::Semicolon, ";");
            return Stmt::make_expr(move(call));
        }
        else
            throw runtime_error("unexpected token after identifier: " + cur.text);
    }

    // literal expr stmt
    if(cur.kind==TokenKind::String || cur.kind==TokenKind::Number || cur.kind==TokenKind::Boolean || cur.kind==TokenKind::Nil)
    {
        auto e = parse_expression();
        expect(TokenKind::Semicolon, ";");
        return Stmt::make_expr(move(e));
    }

    throw runtime_error("unsupported or unexpected token in statement");
}

static unique_ptr<Expr> parse_expression_prec(Parser &p, int min_prec);

unique_ptr<Expr> Parser::parse_expression()
{
    return parse_expression_prec(*this, 0);
}

unique_ptr<Expr> Parser::parse_primary()
{
    // prefix unary (like !, - , ++x, --x)
    if(facts::is_prefix(cur.kind))
    {
        TokenKind op = cur.kind;
        string opname = cur.text;
        eat();

        auto rhs = parse_expression_prec(*this, facts::get_precedence(op));

        vector<unique_ptr<Expr>> args;
        args.push_back(move(rhs));
        return make_unique<Expr>(opname, move(args));
    }

    if(cur.kind == TokenKind::Boolean)
    {
        bool b = cur.text == "true"; eat();
        return make_unique<Expr>(b);
    }
    if(cur.kind == TokenKind::Nil)
    {
        eat();
        return make_unique<Expr>();
    }
    if(cur.kind == TokenKind::Number)
    {
        double n = stod(cur.text); eat();
        return make_unique<Expr>(n);
    }
    if(cur.kind == TokenKind::String)
    {
        string s = cur.text; eat();
        return make_unique<Expr>(s, true);
    }
    if(cur.kind == TokenKind::Identifier)
    {
        string id = cur.text; eat();
        if(cur.kind == TokenKind::LParen) return parse_call_expr(id);

        // identifier could be dotted (io.print) — already lexed as one id
        auto e = make_unique<Expr>();
        e->kind = Expr::KIdent;
        e->ident = id;
        return e;
    }
    if(cur.kind == TokenKind::LParen)
    {
        // disambiguate: function-literal ( (params) stmts end ) vs grouping ( (expr) )
        // We'll do a conservative lookahead using a copy of the lexer to see if it looks like a function literal.
        auto saved_lex = lex;
        Token saved_cur = cur;

        // simulate consuming '('
        Token temp = saved_lex.next(); // now token after '('
        bool looks_like_params = false;
        if(temp.kind == TokenKind::RParen) {
            // empty param list possible -> might be func literal
            looks_like_params = true;
        } else {
            if(temp.kind == TokenKind::Identifier) {
                // scan a simple param-list pattern ident (, ident)* )
                looks_like_params = true;
                // consume params
                while(temp.kind == TokenKind::Identifier) {
                    temp = saved_lex.next();
                    if(temp.kind == TokenKind::Comma) temp = saved_lex.next();
                    else break;
                }
                // after the loop, temp should be RParen for param-list
                if(temp.kind != TokenKind::RParen) looks_like_params = false;
            } else {
                looks_like_params = false;
            }
        }
        // if we saw params-like and after RParen the next token is a statement keyword, decide func-literal
        if(looks_like_params) {
            temp = saved_lex.next(); // token after RParen
            if(temp.kind == TokenKind::Kw_end || temp.kind == TokenKind::Kw_local ||
               temp.kind == TokenKind::Kw_if || temp.kind == TokenKind::Kw_while ||
               temp.kind == TokenKind::Kw_foreach || temp.kind == TokenKind::Kw_return) {
                // function literal
                return parse_func_literal();
            }
            // else fallthrough -> grouping
        }

        // grouping: ( expr )
        eat(); // consume '('
        auto inner = parse_expression();
        expect(TokenKind::RParen, ")");
        return inner;
    }
    throw runtime_error(string("parse expr error at token '") + cur.text + "'");
}

unique_ptr<Expr> Parser::parse_call_expr(const string &name)
{
    expect(TokenKind::LParen, "(");
    vector<unique_ptr<Expr>> args;
    if(cur.kind != TokenKind::RParen)
    {
        args.push_back(parse_expression());
        while(cur.kind == TokenKind::Comma)
        {
            eat();
            args.push_back(parse_expression());
        }
    }
    expect(TokenKind::RParen, ")");
    auto e = make_unique<Expr>(name, move(args));
    return e;
}

unique_ptr<Expr> Parser::parse_func_literal()
{
    expect(TokenKind::LParen, "(");
    vector<string> params;
    if(cur.kind != TokenKind::RParen)
    {
        if(cur.kind == TokenKind::Identifier)
        {
            params.push_back(cur.text); eat();
            while(cur.kind == TokenKind::Comma)
            {
                eat();
                if(cur.kind==TokenKind::Identifier)
                {
                    params.push_back(cur.text);
                    eat();
                }
                else throw runtime_error("expected param name");
            }
        }
    }
    expect(TokenKind::RParen, ")");
    vector<unique_ptr<Stmt>> body;
    while(cur.kind != TokenKind::Kw_end)
        body.push_back(parse_statement());
    
    expect(TokenKind::Kw_end, "end");
    auto e = make_unique<Expr>();
    e->kind = Expr::KFuncLiteral;
    e->params = params;
    e->body = move(body);
    return e;
}

static unique_ptr<Expr> parse_call_or_member_or_index(Parser &p, unique_ptr<Expr> left)
{
    // cur is '(' or '['
    while(true)
    {
        if(p.cur.kind == TokenKind::LParen)
        {
            if(left->kind == Expr::KIdent)
            {
                string callee = left->ident;
                auto call = p.parse_call_expr(callee);
                left = move(call);
            }
            else
            {
                p.expect(TokenKind::LParen, "(");
                vector<unique_ptr<Expr>> args;
                if(p.cur.kind != TokenKind::RParen)
                {
                    args.push_back(p.parse_expression());
                    while(p.cur.kind == TokenKind::Comma)
                    {
                        p.eat();
                        args.push_back(p.parse_expression());
                    }
                }
                
                p.expect(TokenKind::RParen, ")");
                vector<unique_ptr<Expr>> callargs;

                callargs.push_back(move(left));
                for(auto &a : args) callargs.push_back(move(a));

                auto e = make_unique<Expr>("<call>", move(callargs));
                left = move(e);
            }
            continue;
        }
        else if(p.cur.kind == TokenKind::LBracket)
        {
            p.eat();
            auto idx = p.parse_expression();
            p.expect(TokenKind::RBracket, "]");
            vector<unique_ptr<Expr>> args;
            args.push_back(move(left));
            args.push_back(move(idx));
            left = make_unique<Expr>("[index]", move(args));
            continue;
        }
        break;
    }
    return left;
}

static unique_ptr<Expr> parse_expression_prec(Parser &p, int min_prec)
{
    auto left = p.parse_primary();
    while(true)
    {
        if(p.cur.kind == TokenKind::LParen || p.cur.kind == TokenKind::LBracket)
        {
            left = parse_call_or_member_or_index(p, move(left));
            continue;
        }

        TokenKind tok = p.cur.kind;
        int prec = facts::get_precedence(tok);
        if(prec == 0) break;

        if(facts::is_postfix(tok))
        {
            if(prec < min_prec) break;
            string opname = p.cur.text;
            p.eat();
            vector<unique_ptr<Expr>> args;
            args.push_back(move(left));
            left = make_unique<Expr>(opname, move(args));
            continue;
        }
        if(facts::is_infix(tok))
        {
            if(prec < min_prec) break;
            int next_min_prec = prec;

            if(!facts::is_right_associative(tok))
                    next_min_prec = prec + 1;
            else    next_min_prec = prec;

            string opname = p.cur.text;
            p.eat();

            auto right = parse_expression_prec(p, next_min_prec);
            vector<unique_ptr<Expr>> args;
            args.push_back(move(left));
            args.push_back(move(right));
            left = make_unique<Expr>(opname, move(args));
            continue;
        }
        break;
    }

    return left;
}
