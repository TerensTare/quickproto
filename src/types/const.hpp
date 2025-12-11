
#pragma once

#include "types/value.hpp"

struct assign_to_const_type final : value
{
    // TODO: find a better name
    char const *name() const noexcept { return "<assign-to-const>"; }
};

struct const_type final : value
{
    explicit const_type(value const *sub)
        : sub{sub} {}

    // TODO: pass more info
    value const *assign(value const *rhs) const noexcept { return new assign_to_const_type{}; }

    // TODO: find a better name
    char const *name() const noexcept { return "const"; }

    value const *sub;
};