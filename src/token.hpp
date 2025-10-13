
#pragma once

#include <cstdint>
#include "base.hpp"

enum class token_kind : uint8_t
{
    LeftParen,  // (
    RightParen, // )
    LeftBrace,  // {
    RightBrace, // }

    Plus,       // +
    PlusPlus,   // ++
    PlusEqual,  // +=
    Minus,      // -
    MinusMinus, // --
    MinusEqual, // -=
    Star,       // *
    StarEqual,  // *=
    Slash,      // /
    SlashEqual, // /=

    And,      // &
    AndAnd,   // &&
    AndEqual, // &=
    Or,       // |
    OrOr,     // ||
    OrEqual,  // |=
    Xor,      // ^
    XorEqual, // ^=

    Comma,     // ,
    Semicolon, // ;
    Colon,     // :
    Walrus,    // :=

    Bang,         // !
    BangEqual,    // !=
    Equal,        // =
    EqualEqual,   // ==
    Greater,      // >
    GreaterEqual, // >=
    Less,         // <
    LessEqual,    // <=

    // Keywords
    KwBreak,
    KwContinue,
    KwElse,
    KwFalse,
    KwFor,
    KwFunc,
    KwIf,
    KwNil,
    KwReturn,
    KwTrue,
    KwVar,

    // Other variable-length tokens
    Integer,
    Decimal,
    String,
    Ident,

    Eof,
    Unknown,
};

struct token final
{
    constexpr uint32_t finish() const noexcept { return start + len; }

    token_kind kind;
    uint32_t start, len;
};