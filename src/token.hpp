
#pragma once

#include <cstdint>
#include "base.hpp"

enum class token_kind : uint8_t
{
    Eof,

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

    Lshift,      // <<
    LshiftEqual, // <<=
    Rshift,      // >>
    RshiftEqual, // >>=

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
    KwRange,
    KwReturn,
    KwStruct,
    KwTrue,
    KwType,
    KwVar,

    // Other variable-length tokens
    // TODO: (maybe) have different tokens for different int bases, then merge them into a single value type when parsing
    Integer,
    Decimal,
    Rune,
    String,
    Ident,

    // TODO: find a way to compress errors, maybe based on token type
    Unknown,
    // TODO: bad number errors
    BadDigit, // invalid digit (eg. 2) in literal
    BadBase,  // unknown base in number literal
    LongRune,
    UnclosedString, // strings cannot contain newlines, you should use raw string literals instead
    UnicodeIdent,   // unicode characters cannot appear in names
    Count,          // meta; just to get number of tokens
};

static constexpr auto token_to_error = []
{
    auto constexpr N = size_t(token_kind::Count) - size_t(token_kind::Unknown);
    std::array<std::string_view, N> messages;
    for (size_t i{}; i < N; ++i)
        messages[i] = "";

    // TODO: probably these can be broken into "error" in "token kind"
    messages[(size_t)token_kind::Unknown - (size_t)token_kind::Unknown] = "Unknown character";
    messages[(size_t)token_kind::BadDigit - (size_t)token_kind::Unknown] = "Invalid digit in number literal";
    messages[(size_t)token_kind::BadBase - (size_t)token_kind::Unknown] = "Unknown base in number literal";
    messages[(size_t)token_kind::LongRune - (size_t)token_kind::Unknown] = "Runes can only contain one character";
    messages[(size_t)token_kind::UnclosedString - (size_t)token_kind::Unknown] = "Unterminated string";
    messages[(size_t)token_kind::UnicodeIdent - (size_t)token_kind::Unknown] = "Names cannot contain unicode characters. They can only appear in strings and comments.";

    for (size_t i{}; i < N; ++i)
        if (messages[i] == "")
            throw std::exception{"Internal compiler error: message not specified for error token"};

    return messages;
}();

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
    hashed_name hash;    // present on keyword/ident, but do not rely on it for keyword detection
    uint32_t start, len; // starting byte + number of bytes in the source code
};

static_assert(sizeof(token) == sizeof(uint64_t[2]));