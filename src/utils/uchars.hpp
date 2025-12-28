
#pragma once

#include "base.hpp"

constexpr bool is_alpha(uchar ch) noexcept
{
    return (ch >= 'A' && ch <= 'Z') ||
           (ch >= 'a' && ch <= 'z') ||
           (ch == '_');
}

constexpr bool is_bin(uchar ch) noexcept { return (ch & 0xFE) == 48; } // '0' or '1'
constexpr bool is_oct(uchar ch) noexcept { return ch >= '0' && ch <= '7'; }
constexpr bool is_dec(uchar ch) noexcept { return ch >= '0' && ch <= '9'; }

constexpr bool is_hex(uchar ch) noexcept
{
    return (ch >= '0' && ch <= '9') ||
           (ch >= 'A' && ch <= 'F') ||
           (ch >= 'a' && ch <= 'f');
}

constexpr bool is_alphanum(uchar ch) noexcept { return is_alpha(ch) || is_dec(ch); }

constexpr bool is_utf8(uchar ch) noexcept { return ch & 0x80; }

// invariant: ch is a valid encoded UTF8 byte and the first of the character
// TODO: is array better here?
constexpr size_t utf8_step(uchar ch) noexcept
{
    switch (ch >> 4)
    {
    case 0b1100:
        return 2;
    case 0b1110:
        return 3;
    case 0b1111:
        return 4;
    default:
        return 1; // ascii
    }
}

struct uchars final
{
    // Get the current byte.
    constexpr auto operator*() const noexcept { return *chars; }

    // Go to the next character and return the old, when you know that this is an ASCII character (ie. < 0x80).
    constexpr auto next_ascii() noexcept { return *chars++; }

    // Go to the next character, no matter if this is a ASCII or UTF8 character.
    constexpr void next_utf8() noexcept { chars += utf8_step(*chars); }

    // Advance the pointer if the given ASCII character is the current one, otherwise do nothing
    constexpr bool eat(uchar ch) noexcept
    {
        if (*chars == ch)
        {
            ++chars;
            return true;
        }
        return false;
    }

    uchar const *chars;
};