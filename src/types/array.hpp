
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

struct array_type final : type
{
    inline array_type(type const *base, size_t n) noexcept
        : base{base}, n{n} {}

    inline value const *top() const noexcept;

    type const *base;
    size_t n; // number of elements; for now this must be known at compile time
};

struct array_value final : value
{
    inline explicit array_value(array_type const *type) noexcept
        : type{type} {}

    inline value const *assign(value const *rhs) const noexcept
    {
        // if (auto arr = rhs->as<array_value>())
        // {
        //     if (auto subres = type->assign(arr->base);
        //         subres->as<value_error>())
        //         return subres; // TODO: compose the error/return a custom one

        //     if (n != arr->n)
        //         return new different_size_array{n, arr->n};

        //     // TODO: actually deduce the most-defined-sized
        //     return rhs;
        // }
        // else
        // TODO: implement
            return value::assign(rhs);
    }

    inline value const *index(value const *i) const noexcept
    {
        // NOTE: this assumes `i` is within bounds, which should be checked before calling this operation
        return i->as<int_value>()
                   ? type->base->top()
                   : value::index(i);
    }

    // TODO: return a better name
    inline char const *name() const noexcept { return "<array>"; }

    array_type const *type;
};

inline value const *array_type::top() const noexcept { return new array_value{this}; }