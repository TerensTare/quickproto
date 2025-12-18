
#pragma once

#include "types/value.hpp"

// TODO: implement other operators eventually
struct nil_value final : value
{
    inline char const *name() const noexcept { return "nil"; }
};

struct pointer_value final : value
{
    type const *ty;
};

// TODO: implement
struct pointer_type final : type
{
    inline explicit pointer_type(type const *base) noexcept
        : base{base} {}

    // TODO: cache the value
    inline value const *top() const noexcept { return new nil_value{}; }

    type const *base;
};
