
#pragma once

#include "types/value.hpp"

struct string_type final : type
{
    inline string_type() noexcept {}

    inline value const *top() const noexcept;
    inline value const *zero() const noexcept;

    // TODO: denote the name better
    inline char const *name() const noexcept { return "string"; }
};

// TODO: str + str
struct string_top final : value
{
    inline string_top() noexcept {}

    inline value const *assign(value const *rhs) const noexcept
    {
        // TODO: implement
        return value::assign(rhs);
    }
};

// TODO: keep string info here
struct string_value final : value
{
    inline string_value() noexcept {}
};

inline value const *string_type::top() const noexcept { return new string_top; }
// TODO: this should give a zero-string instead, address this
inline value const *string_type::zero() const noexcept { return new string_value; }