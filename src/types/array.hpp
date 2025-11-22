
#pragma once

#include "types/int.hpp"
#include "types/value.hpp"

// TODO: actually store each element's type for later optimizations
// TODO: also store a `common type` for fast computations
// TODO: remember you will probably have runtime-sized arrays at some point

struct different_size_array final : value_error
{
    inline different_size_array(size_t lhs, size_t rhs)
        : lhs{lhs}, rhs{rhs} {}

    inline char const *name() const noexcept { return "<different-size-array>"; }

    size_t lhs, rhs;
};

struct array_type final : value_type
{
    inline array_type(value_type const *base, size_t n)
        : base{base}, n{n} {}

    inline value_type const *assign(value_type const *rhs) const noexcept
    {
        if (auto arr = rhs->as<array_type>())
        {
            if (auto subres = base->assign(arr->base);
                subres->as<value_error>())
                return subres; // TODO: compose the error/return a custom one

            if (n != arr->n)
                return new different_size_array{n, arr->n};

            // TODO: actually deduce the most-defined-sized
            return rhs;
        }
        else
            return value_type::assign(rhs);
    }

    inline value_type const *index(value_type const *i) const noexcept
    {
        // NOTE: this assumes `i` is within bounds, which should be checked before calling this operation
        return i->as<int_>()
                   ? base
                   : value_type::index(i);
    }

    // TODO: return a better name
    inline char const *name() const noexcept { return "<array>"; }

    value_type const *base;
    size_t n; // number of elements; for now this must be known at compile time
};