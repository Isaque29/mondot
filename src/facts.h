#ifndef MONDOT_FACTS_H
#define MONDOT_FACTS_H

#include "lexer.h"

namespace facts
{
    int get_precedence(TokenKind k);
    bool is_right_associative(TokenKind k);
    bool is_postfix(TokenKind k);
    bool is_prefix(TokenKind k);
    bool is_infix(TokenKind k);
}

#endif
