
#pragma once

#include <unordered_map>
#include <string_view>

#include "token.hpp"

// TODO:
// - implicit semicolon insertion

constexpr bool is_alpha(char ch) noexcept
{
    return (ch >= 'A' && ch <= 'Z') ||
           (ch >= 'a' && ch <= 'z') ||
           (ch == '_');
}

constexpr bool is_dec(char ch) noexcept
{
    return ch >= '0' && ch <= '9';
}

constexpr bool is_alphanum(char ch) noexcept
{
    return is_alpha(ch) || is_dec(ch);
}

struct scanner final
{
    inline token next() noexcept;

    constexpr auto lexeme(token const &t) const noexcept -> std::string_view
    {
        return {text + t.start, t.len};
    }

    char const *text;
    token peek = bootstrap();

private:
    inline token bootstrap() noexcept
    {
        peek = {.kind = token_kind::LeftParen};
        return next();
    }
};

struct scanner_iter final
{
    template <bool InsertSemicolon>
    inline void skip_ws() noexcept;

    inline token_kind next() noexcept;

    constexpr bool eat(char c) noexcept
    {
        if (*text == c)
        {
            ++text;
            return true;
        }
        else
            return false;
    }

    char const *text;
};

// TODO: try this vs entt::dense_map
inline static std::unordered_map<std::string_view, token_kind> const keywords{
    {"break", token_kind::KwBreak},
    {"const", token_kind::KwConst},
    {"continue", token_kind::KwContinue},
    {"else", token_kind::KwElse},
    {"false", token_kind::KwFalse},
    {"for", token_kind::KwFor},
    {"func", token_kind::KwFunc},
    {"if", token_kind::KwIf},
    {"nil", token_kind::KwNil},
    {"return", token_kind::KwReturn},
    {"true", token_kind::KwTrue},
    {"var", token_kind::KwVar},
};

inline bool auto_semicolon(token_kind kind)
{
    switch (kind)
    {
    // TODO: float + any kind of literal
    case token_kind::Integer:
    case token_kind::String:
    case token_kind::Ident:
    case token_kind::KwTrue:
    case token_kind::KwFalse:
    case token_kind::KwNil:
    case token_kind::KwBreak:
    case token_kind::KwContinue:
    case token_kind::KwReturn:
    case token_kind::PlusPlus:
    case token_kind::MinusMinus:
    case token_kind::RightParen:
    case token_kind::RightBrace:
        // TODO: rightBracket
        return true;

    default:
        return false;
    }
}

inline token scanner::next() noexcept
{
    auto const old = peek;

    auto iter = scanner_iter{text + peek.finish()};
    auto_semicolon(old.kind)
        ? iter.skip_ws<true>()
        : iter.skip_ws<false>();

    auto const start = uint32_t(iter.text - text);
    auto const next = iter.next();

    peek = token{
        .kind = next,
        .start = start,
        .len = uint32_t(iter.text - text) - start,
    };

    if (auto const iter = keywords.find(lexeme(peek)); iter != keywords.end())
    {
        peek.kind = iter->second;
    }

    return old;
}

template <bool InsertSemicolon>
inline void scanner_iter::skip_ws() noexcept
{
    switch (*text)
    {
    case '\0':
        return;

    case '/':
        switch (text[1])
        {
        case '/':
            text += 2; // '//'
            goto line_comment;
        case '*':
            text += 2; // '/*'
            goto multiline_comment;
        default:
            return;
        }

    default:
        if constexpr (InsertSemicolon)
        {
            // automatic semicolon insertion
            if (*text == '\n')
                return;
        }

        if (*text <= ' ')
        {
            ++text;

            while (*text <= ' ')
            {
                if constexpr (InsertSemicolon)
                {
                    // automatic semicolon insertion
                    if (*text == '\n')
                        return;
                }

                ++text;
            }

            return skip_ws<InsertSemicolon>();
        }
        else
            return;
    }

    return; // just in case

line_comment:
    while (true)
    {
        switch (*text)
        {
        case '\n':
            // automatic semicolon insertion
            if constexpr (InsertSemicolon)
                return;

            else
            {
                ++text;
                // invariant: this is unreachable when `InsertSemicolon` is `true`
                return skip_ws<false>();
            }

        case '\0':
            return;

        default:
            ++text;
        }
    }

multiline_comment:
    const char *next = nullptr;
    while (next = strchr(text, '*'))
    {
        text = next + 1;
        if (*text == '/')
        {
            ++text;
            return skip_ws<InsertSemicolon>();
        }
    }

    text += strlen(text); // TODO: does this go to '\0'?
}

inline token_kind scanner_iter::next() noexcept
{
    using enum token_kind;

    switch (auto ch = *text++; ch)
    {
    case '\0':
        return Eof;

    case '(':
        return LeftParen;
    case ')':
        return RightParen;
    case '{':
        return LeftBrace;
    case '}':
        return RightBrace;

    case '+':
        return eat('+')
                   ? PlusPlus
               : eat('=') ? PlusEqual
                          : Plus;

    case '-':
        return eat('-')
                   ? MinusMinus
               : eat('=') ? MinusEqual
                          : Minus;

    case '*':
        return eat('=') ? StarEqual : Star;
    case '/':
        return eat('=') ? SlashEqual : Slash;

    case ',':
        return Comma;

    case '\n': // this is only hit on automatic semicolon insertion
    case ';':
        return Semicolon;

    case ':':
        return eat('=')
                   ? Walrus
                   : Colon;

    case '!':
        return eat('=')
                   ? BangEqual
                   : Bang;

    case '=':
        return eat('=')
                   ? EqualEqual
                   : Equal;

    case '>':
        return eat('=')
                   ? GreaterEqual
                   : Greater;

    case '<':
        return eat('=')
                   ? LessEqual
                   : Less;

    case '&':
        return eat('&')
                   ? AndAnd
               : eat('=') ? AndEqual
                          : And;

    case '|':
        return eat('|')
                   ? OrOr
               : eat('=') ? OrEqual
                          : Or;

    case '^':
        return eat('=')
                   ? XorEqual
                   : Xor;

    case '"':
        // TODO: error on newline
        while (*text && *text != '"')
            ++text;

        return String;

    default:
        if (is_alpha(ch))
        {
            while (is_alphanum(*text))
                ++text;

            return Ident;
        }
        else if (is_dec(ch))
        {
            while (is_dec(*text))
                ++text;

            if (eat('.'))
            {
                while (is_dec(*text))
                    ++text;

                return Decimal;
            }

            return Integer;
        }

        return Unknown;
    }
}
