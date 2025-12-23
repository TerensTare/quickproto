
#pragma once

#include "types/value.hpp"

// TODO: implement other operators eventually
struct nil_value final : value
{
    inline static nil_value const *self() noexcept
    {
        static nil_value nil;
        return &nil;
    }
};

struct pointer_value final : value
{
    inline explicit pointer_value(type const *ty) noexcept : ty{ty} {}

    type const *ty;
};

// TODO: implement
struct pointer_type final : type
{
    inline explicit pointer_type(type const *base) noexcept
        : base{base} {}

    // TODO: cache the value
    inline value const *top() const noexcept { return new pointer_value{this}; }
    inline value const *zero() const noexcept { return new nil_value{}; }
    // TODO: also show the base type
    inline char const *name() const noexcept { return "{pointer}"; }

    type const *base;
};
