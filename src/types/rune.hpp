
#pragma once

#include "types/value.hpp"

struct rune_type final : type
{
    inline rune_type() noexcept {}

    inline value const *top() const noexcept;
    inline value const *zero() const noexcept;

    // TODO: denote the name better
    inline char const *name() const noexcept { return "rune"; }
};

struct rune_top final : value
{
    inline rune_top() noexcept {}

    // TODO: more operators
    inline value const *assign(value const *rhs) const noexcept
    {
        // TODO: implement
        return value::assign(rhs);
    }
};

struct rune_value final : value
{
    // TODO: `rune` is an alias to `int32`, address that
    inline explicit rune_value(int32_t ch) noexcept {}

    int32_t ch;
};

inline value const *rune_type::top() const noexcept { return new rune_top; }
// TODO: this should give a zero-rune instead, address this
inline value const *rune_type::zero() const noexcept { return new rune_value{0}; }