
#pragma once

#include <cstdint>
#include "base.hpp"

enum class token_kind : uint8_t
{
    LeftParen,    // (
    RightParen,   // )
    LeftBracket,  // [
    RightBracket, // ]
    LeftBrace,    // {
    RightBrace,   // }

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

    At,        // @
    Comma,     // ,
    Dot,       // .
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
    KwConst,
    KwContinue,
    KwDefer,
    KwElse,
    KwFalse,
    KwFor,
    KwFunc,
    KwIf,
    KwNil,
    KwPackage,
    KwReturn,
    KwStruct,
    KwTrue,
    KwType,
    KwVar,

    // Other variable-length tokens
    Integer,
    Decimal,
    String,
    Ident,

    Eof,
    Unknown,
};

enum class hashed_name : std::uint32_t
{
};

// NOTE: despite the name, this hashes a `hashed_name`, not a `token`
struct token_hash final
{
    static constexpr size_t operator()(hashed_name h) noexcept { return (uint32_t)h; }
};

struct token final
{
    constexpr uint32_t finish() const noexcept { return start + len; }

    // TODO: if you have the hash + kind you don't really need the length (you do for integers, etc. tho)
    // maybe it's best to pre-parse integers/double etc. in a uint64/double rather than keep the length, not much value in it
    token_kind kind;
    hashed_name hash; // present on keyword/ident, but do not rely on it for keyword detection
    uint32_t start, len;
};

static_assert(sizeof(token) == sizeof(uint64_t[2]));