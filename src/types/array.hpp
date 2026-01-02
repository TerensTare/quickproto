
#pragma once

#include "types/int.hpp"
#include "types/composite.hpp"

// TODO: actually store each element's type for later optimizations
// TODO: also store a `common type` for fast computations
// TODO: remember you will probably have runtime-sized arrays at some point

struct array_type final : composite_type
{
    // for now the number of elements must be known at compile time
    inline array_type(type const *base, size_t n) noexcept
        : composite_type{n}, base{base} {}

    inline value const *top() const noexcept;
    inline value const *zero() const noexcept;
    inline composite_value const *init(value const **values) const noexcept;

    // TODO: denote the name better
    inline char const *name() const noexcept { return "{array}"; }

    type const *base;
};

struct different_size_array final : value_error
{
    inline different_size_array(size_t lhs, size_t rhs)
        : lhs{lhs}, rhs{rhs} {}

    size_t lhs, rhs;
};

struct out_of_bounds final : value_error
{
    inline out_of_bounds(array_type const *type, int_const const *index)
        : type{type}, idx{index} {}

    array_type const *type;
    int_const const *idx;
};

struct array_value final : composite_value
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
        if (auto int_i = i->as<int_value>())
        {
            // TODO: handle the runtime-sized array case
            if (auto c_i = int_i->as<int_const>(); c_i->n >= type->n_members)
                return new out_of_bounds{type, c_i};

                // TODO: return the actual value, if known
            return type->base->top();
        }
        else
            return value::index(i);
    }

    array_type const *type;
};

inline value const *array_type::top() const noexcept { return new array_value{this}; }
// TODO: every index should be zero-init instead; address this
inline value const *array_type::zero() const noexcept { return new array_value{this}; }

// TODO: every index should be value-init instead; address this
inline composite_value const *array_type::init(value const **values) const noexcept
{
    return new array_value{this};
}