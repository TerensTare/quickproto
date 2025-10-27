
#pragma once

#include "types/value.hpp"

struct assign_to_const_type final : value_type
{
    // TODO: find a better name
    char const *name() const noexcept { return "<assign-to-const>"; }
};

struct const_type final : value_type
{
    explicit const_type(value_type const *sub)
        : sub{sub} {}

    // TODO: pass more info
    value_type const *assign(value_type const *rhs) const noexcept { return new assign_to_const_type{}; }

    // TODO: find a better name
    char const *name() const noexcept { return "const"; }

    value_type const *sub;
};