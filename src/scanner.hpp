
#pragma once

#include <string_view>
#include <unordered_map>

#include "token.hpp"
#include "utils/uchars.hpp"

// TODO:
// - flag identifiers that start as uppercase
// - parse integer values as `uint64`, then add `truncate` nodes if specified type is smaller
// ^ what about negative numbers though?
// ^ not a problem as initially `-` and `{integer}` are separate tokens
// ^ repeat the same logic for floating points with `float64`, and also for assigning int constants to floating values
// - keep track of the whole text size (you already have it) and the remaining characters, so you can do vectorized search

struct scanner final
{
    inline token next() noexcept;

    constexpr auto lexeme(token const &t) const noexcept -> std::string_view
    {
        return {(char const *)text + t.start, t.len};
    }

    uchar const *text;
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
    inline token_kind next(hashed_name &name) noexcept;

    // TODO: do you need this anymore?
    constexpr bool eat(uchar c) noexcept { return text.eat(c); }

    // helper rules
    template <bool InsertSemicolon>
    inline void skip_ws() noexcept;

    // bin ::= '0' ('b' | 'B') b* - where b ::= '0' | '1'
    // oct ::= '0' ('o' | 'O')? o* - where o ::= '0'-'7'
    // hex ::= '0' ('x' | 'X') h* - where h ::= '0'-'9' | 'a'-'f' | 'A'-'F'
    // flt ::= '0' '.' d* - where d ::= '0'-'9'
    inline token_kind zero_rule() noexcept;

    // identifier or keyword
    inline token_kind ident_or_kw(hashed_name &name) noexcept;

    // only identifier
    inline token_kind ident(hashed_name &name) noexcept;

    uchars text;
};

// TODO: try this vs entt::dense_map
inline static std::unordered_map<std::string_view, token_kind> const keywords{
    {"break", token_kind::KwBreak},
    {"const", token_kind::KwConst},
    {"continue", token_kind::KwContinue},
    {"defer", token_kind::KwDefer},
    {"else", token_kind::KwElse},
    {"false", token_kind::KwFalse},
    {"for", token_kind::KwFor},
    {"func", token_kind::KwFunc},
    {"if", token_kind::KwIf},
    {"nil", token_kind::KwNil},
    {"package", token_kind::KwPackage},
    {"range", token_kind::KwRange},
    {"return", token_kind::KwReturn},
    {"struct", token_kind::KwStruct},
    {"true", token_kind::KwTrue},
    {"type", token_kind::KwType},
    {"var", token_kind::KwVar},
};

inline bool auto_semicolon(token_kind kind)
{
    switch (kind)
    {
    case token_kind::Integer:
    case token_kind::Decimal:
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
    case token_kind::RightBracket:
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

    auto const start = uint32_t(iter.text.chars - text);
    hashed_name hash{};
    auto const next = iter.next(hash);

    peek = token{
        .kind = next,
        .hash = hash,
        .start = start,
        .len = uint32_t(iter.text.chars - text) - start,
    };

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
        text.next_ascii();

        switch (*text)
        {
        case '/':
            text.next_ascii(); // '//'
            goto line_comment;

        case '*':
            text.next_ascii(); // '/*'
            goto multiline_comment;

        default:
            --text.chars;
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
            text.next_ascii();

            while (*text <= ' ')
            {
                if constexpr (InsertSemicolon)
                {
                    // automatic semicolon insertion
                    if (*text == '\n')
                        return;
                }

                text.next_ascii();
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
        // TODO: only ascii bytes ever have the leading 0, so is it ever better here to just do a linear scan and avoid the variable step?
        switch (*text)
        {
        case '\n':
            // automatic semicolon insertion
            if constexpr (InsertSemicolon)
                return;

            else
            {
                text.next_ascii();
                // invariant: this is unreachable when `InsertSemicolon` is `true`
                return skip_ws<false>();
            }

        case '\0':
            return;

        default:
            text.next_utf8();
        }
    }

multiline_comment:
    // TODO: optimize this
    // TODO: only ascii bytes ever have the leading 0, so is it ever better here to just do a linear scan and avoid the variable step?
    switch (*text)
    {
    case '\0':
        return;

    case '*':
        text.next_ascii();
        if (text.eat('/'))
            return skip_ws<InsertSemicolon>();

        goto multiline_comment;

    default:
        text.next_utf8();
        goto multiline_comment;
    }
}

inline token_kind scanner_iter::zero_rule() noexcept
{
    using enum token_kind;

    // TODO: parse _ in between
    switch (*text)
    {
        // binary
    case 'b':
    case 'B':
    {
        text.next_ascii();

        // binary
        while (is_bin(*text))
            text.next_ascii();

        // TODO: is this correct? (any other characters that trigger the error?)
        if (is_hex(*text))
        {
            do
                text.next_ascii();
            while (is_hex(*text));

            return BadDigit;
        }

        return Integer;
    }

    // hex
    case 'x':
    case 'X':
    {
        text.next_ascii();

        // hex
        while (is_hex(*text))
            text.next_ascii();

        // TODO: report bad characters

        return Integer;
    }

    // flt
    case '.':
    {
        // TODO: scientific notation, etc.
        text.next_ascii();

        while (is_dec(*text))
            text.next_ascii();

        return Decimal;
    }

    // oct
    case 'o':
    case 'O':
        text.next_ascii(); // o/O

        if (is_oct(*text))
            goto oct;
        else // there should be at least 1 octal digit after the o/O
            return BadBase;

    default:
        if (is_oct(*text))
            goto oct;
        else if (is_hex(*text))
            return BadBase;
        else
            return Integer; // just 0
    }

oct:
    // octal
    do
        text.next_ascii();
    while (is_oct(*text));

    // TODO: is this correct? (any other characters that trigger the error?)
    if (is_hex(*text))
    {
        do
            text.next_ascii();
        while (is_hex(*text));

        return BadDigit;
    }

    return Integer;
}

inline token_kind scanner_iter::ident_or_kw(hashed_name &hash) noexcept
{
    using enum token_kind;

    auto const start = text.chars - 1;

    // TODO: can you optimize this further?
    token_kind ret = Ident;

    while (auto ch = *text)
    {
        if (is_alphanum(ch))
            text.next_ascii();
        else if (is_utf8(ch))
        {
            text.next_utf8();
            goto unicode_ident_or_kw;
        }
        else
            break;
    }

    {
        // TODO: do something more efficient here (you get the hash of this identifier twice: once for the token and once for the lookup)
        auto const str = std::string_view{(char const *)start, (char const *)text.chars};

        if (auto const iter = keywords.find(str); iter != keywords.end())
            ret = iter->second;

        // invariant: this part of the string is ASCII
        hash = (hashed_name)entt::basic_hashed_string<uchar>::value(start, text.chars - start);

        return ret;
    }

unicode_ident_or_kw:
    while (auto ch = *text)
    {
        if (is_alphanum(ch))
            text.next_ascii();
        else if (is_utf8(ch))
            text.next_utf8();
        else
            break;
    }

    return UnicodeIdent;
}

inline token_kind scanner_iter::ident(hashed_name &hash) noexcept
{
    using enum token_kind;

    auto const start = text.chars - 1;

    // TODO: is it best to have separate loops for this?
    while (auto ch = *text)
    {
        if (is_alphanum(ch))
            text.next_ascii();
        else if (is_utf8(ch))
        {
            text.next_utf8();
            goto unicode_ident;
        }
        else
            break;
    }

    // invariant: identifiers are always ASCII
    hash = (hashed_name)entt::basic_hashed_string<uchar>::value(start, text.chars - start);

    return Ident;

unicode_ident:
    while (auto ch = *text)
    {
        if (is_alphanum(ch))
            text.next_ascii();
        else if (is_utf8(ch))
            text.next_utf8();
        else
            break;
    }

    return UnicodeIdent;
}

inline token_kind scanner_iter::next(hashed_name &hash) noexcept
{
    using enum token_kind;

    switch (auto ch = text.next_ascii(); ch)
    {
    case '\0':
        return Eof;

    case '(':
        return LeftParen;
    case ')':
        return RightParen;
    case '[':
        return LeftBracket;
    case ']':
        return RightBracket;
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

    case '@':
        return At;
    case ',':
        return Comma;
    case '.':
        return Dot;

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

    case '<':
        if (eat('<'))
            return eat('=')
                       ? LshiftEqual
                       : Lshift;
        return eat('=')
                   ? LessEqual
                   : Less;

    case '>':
        if (eat('<'))
            return eat('=')
                       ? RshiftEqual
                       : Rshift;
        return eat('=')
                   ? GreaterEqual
                   : Greater;

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

    // TODO: handle escape sequences (\x, \u)
    case '\'':
        // TODO: probably best to make this into a function to reuse for strings
        switch (*text)
        {
        case '\0':
            return LongRune;

            // TODO: if ', error because of empty rune

        case '\\':
            text.next_ascii();
            // TODO: implement
            switch (*text) {}
            break;

        default:
            text.next_utf8();
        }

        return eat('\'') ? Rune : LongRune;

    case '"':
        // TODO: optimize this
        // TODO: only ascii bytes ever have the leading 0, so is it ever better here to just do a linear scan and avoid the variable step?
        // TODO: handle escape sequences (\", \u, \x, \0-7)
        while (true)
        {
            switch (*text)
            {
            case '\n':
                text.next_ascii();
                [[fallthrough]];

            case '\0':
                return UnclosedString;

            case '"':
                text.next_ascii();
                return String;

            default:
                text.next_utf8();
            }
        }

    // TODO: parse other integer literals (octal, hex)
    // TODO: maybe it's best to lex the number on the go as you need to handle the `_` and keep it in the hash?
    // TODO: move this to a separate function
    case '0':
        return zero_rule();

    default:
        // this might be a keyword
        if (ch >= 'a' && ch <= 'z')
            return ident_or_kw(hash);
        else if (is_alpha(ch)) // but this is not a keyword for sure
            return ident(hash);
        else if (is_dec(ch)) // invariant: ch is never 0 here
        {
            while (is_dec(*text))
                text.next_ascii();

            if (eat('.'))
            {
                while (is_dec(*text))
                    text.next_ascii();

                // TODO: handle scientific notation

                if (is_hex(*text))
                {
                    do
                        text.next_ascii();
                    while (is_hex(*text));

                    return BadDigit;
                }

                return Decimal;
            }
            else if (is_hex(*text))
            {
                do
                    text.next_ascii();
                while (is_hex(*text));

                return BadDigit;
            }

            return Integer;
        }
        else if (is_utf8(ch)) // if UTF8, correctly go to the next character and return unknown either way, as UTF8 characters are only valid in comments or strings.
            text.chars += utf8_step(ch) - 1;

        return Unknown;
    }
}
