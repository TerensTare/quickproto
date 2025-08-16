
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
    KwFalse,
    KwFunc,
    KwNil,
    KwReturn,
    KwTrue,

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