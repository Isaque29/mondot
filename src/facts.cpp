#include "facts.h"

namespace facts
{
    int get_precedence(TokenKind k)
    {
        switch(k)
        {
            // member / call / index: top precedence for chaining
            case TokenKind::LParen:       return 110; // call (handled specially)
            case TokenKind::LBracket:     return 110; // index (handled specially)

            // postfix (higher than unary)
            case TokenKind::PlusPlus:     return 100;
            case TokenKind::MinusMinus:   return 100;

            // unary prefix handled separately but assign a high precedence
            case TokenKind::Exclamation:         return 90;   // '!'

            // multiplicative
            case TokenKind::Star:         return 80;
            case TokenKind::Slash:        return 80;
            case TokenKind::Percent:      return 80;

            // additive
            case TokenKind::Plus:         return 70;
            case TokenKind::Minus:        return 70;

            // comparisons
            case TokenKind::Less:         return 60;
            case TokenKind::LessEqual:    return 60;
            case TokenKind::Greater:      return 60;
            case TokenKind::GreaterEqual: return 60;

            // equality
            case TokenKind::EqualEqual:   return 50;
            case TokenKind::NotEqual:     return 50;

            // logical and / or
            case TokenKind::Ampersand:       return 40; // &
            case TokenKind::Pipe:         return 30; // |
            case TokenKind::Equal:        return 10;

            default:
                return 0;
        }
    }

    bool is_right_associative(TokenKind k)
    {
        switch(k)
        {
            case TokenKind::Equal:        return true; // a = b = c -> right associative
            default:
                return false;
        }
    }

    bool is_postfix(TokenKind k)
    {
        switch(k)
        {
            case TokenKind::PlusPlus:
            case TokenKind::MinusMinus:
                return true;
            default:
                return false;
        }
    }

    bool is_prefix(TokenKind k)
    {
        switch(k)
        {
            case TokenKind::Exclamation:
            case TokenKind::Minus:
            case TokenKind::PlusPlus:
            case TokenKind::MinusMinus:
                return true;
            default:
                return false;
        }
    }

    bool is_infix(TokenKind k)
    {
        return get_precedence(k) > 0 && !is_postfix(k);
    }
}
