
#pragma once

#include <cstdint>
#include "types/bool.hpp"

struct int_ : value_type
{
    inline value_type const *assign(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? rhs
                   : value_type::assign(rhs);
    }

    inline char const *name() const noexcept final { return "int"; }
};

struct int_top final : int_
{
    inline static value_type const *self() noexcept
    {
        static int_top top;
        return &top;
    }

    // impl value_type

    inline value_type const *add(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? rhs
                   : new binary_op_not_implemented_type{"+", this, rhs};
    }

    inline value_type const *sub(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? rhs
                   : new binary_op_not_implemented_type{"-", this, rhs};
    }

    inline value_type const *mul(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? rhs
                   : new binary_op_not_implemented_type{"*", this, rhs};
    }

    inline value_type const *div(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? rhs
                   : new binary_op_not_implemented_type{"/", this, rhs};
    }

    inline value_type const *neg() const noexcept { return this; }

    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? bool_top::self()
                   : new binary_op_not_implemented_type{"==", this, rhs};
    }

    inline value_type const *lt(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? bool_top::self()
                   : new binary_op_not_implemented_type{"<", this, rhs};
    }
};

struct int_bot final : int_
{
    inline static value_type const *self() noexcept
    {
        static int_bot bot;
        return &bot;
    }

    // impl value_type

    inline value_type const *add(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : new binary_op_not_implemented_type{"+", this, rhs};
    }

    inline value_type const *sub(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : new binary_op_not_implemented_type{"-", this, rhs};
    }

    inline value_type const *mul(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : new binary_op_not_implemented_type{"*", this, rhs};
    }

    inline value_type const *div(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : new binary_op_not_implemented_type{"/", this, rhs};
    }

    inline value_type const *neg() const noexcept { return this; }

    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : new binary_op_not_implemented_type{"==", this, rhs};
    }

    inline value_type const *lt(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : new binary_op_not_implemented_type{"<", this, rhs};
    }
};

struct int_const final : int_
{
    explicit int_const(int64_t n) noexcept : n{n} {}

    // impl value_type
    inline value_type const *add(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new int_const{n + ptr->n};
        else if (rhs->as<int_top>())
            return this;
        else
            return new binary_op_not_implemented_type{"+", this, rhs};
    }

    inline value_type const *sub(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new int_const{n - ptr->n};
        else if (rhs->as<int_top>())
            return this;
        else
            return new binary_op_not_implemented_type{"-", this, rhs};
    }

    inline value_type const *mul(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new int_const{n * ptr->n};
        else if (rhs->as<int_top>())
            return this;
        else
            return new binary_op_not_implemented_type{"*", this, rhs};
    }

    inline value_type const *div(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return (ptr->n == 0) ? int_top::self() : new int_const{n / ptr->n};
        else if (rhs->as<int_top>())
            return this;
        else
            return new binary_op_not_implemented_type{"/", this, rhs};
    }

    inline value_type const *neg() const noexcept { return new int_const{-n}; }

    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new bool_const{n == ptr->n};
        else if (rhs->as<int_top>())
            return bool_top::self();
        else
            return new binary_op_not_implemented_type{"==", this, rhs};
    }

    inline value_type const *lt(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new bool_const{n < ptr->n};
        else if (rhs->as<int_top>())
            return bool_top::self();
        else
            return new binary_op_not_implemented_type{"<", this, rhs};
    }

    int64_t n;
};

// TODO: two or more possible ranges (eg. [-5, -3] to [3, 5])
// TODO: lt, gt, le, ge
struct int_range final : int_
{
    inline int_range(int64_t lo = INT64_MIN, int64_t hi = INT64_MAX) noexcept
        : lo{lo}, hi{hi} {}

    int64_t lo, hi;
};
