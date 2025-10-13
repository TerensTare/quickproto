
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

inline static std::unordered_map<std::string_view, token_kind> const keywords{
    {"break", token_kind::KwBreak},
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

inline token scanner::next() noexcept
{
    auto const old = peek;

    auto start = peek.finish();

    auto iter = scanner_iter{text + start};
    iter.skip_ws();

    start = uint32_t(iter.text - text);

    auto const next = iter.next();
    peek = token{
        .kind = next,
        .start = start,
        .len = uint32_t(iter.text - text) - start,
    };

    if (auto iter = keywords.find(lexeme(peek)); iter != keywords.end())
    {
        peek.kind = iter->second;
    }

    return old;
}

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
        if (*text <= ' ')
        {
            ++text;

            while (*text <= ' ')
                ++text;

            return skip_ws();
        }
        else
            return;
    }

    return;

line_comment:
    while (true)
    {
        switch (*text)
        {
        case '\n':
            ++text;
            [[fallthrough]];
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
            // TODO: you can merge this switch here along with the `/` check
            return skip_ws();
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
