
#pragma once

#include "types/int.hpp"
#include "types/value.hpp"

// TODO: actually store each element's type for later optimizations

struct array_type final : value_type
{
    inline array_type(value_type const *base, size_t n)
        : base{base}, n{n} {}

    inline value_type const *index(value_type const *i) const noexcept
    {
        // NOTE: this assumes `i` is within bounds, which should be checked before calling this operation
        return i->as<int_>()
                   ? base
                   : new binary_op_not_implemented_type{"[]", this, i};
    }

    // TODO: return a better name
    inline char const *name() const noexcept { return "<array>"; }

    value_type const *base;
    size_t n; // number of elements; for now this must be known at compile time
};