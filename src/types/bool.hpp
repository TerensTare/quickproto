
#pragma once

#include "types/value.hpp"

struct bool_ : value_type
{
    inline value_type const *assign(value_type const *rhs) const noexcept
    {
        return rhs->as<bool_>()
                   ? rhs
                   : value_type::assign(rhs);
    }

    inline char const *name() const noexcept final { return "bool"; }
};

struct bool_top final : bool_
{
    inline static value_type const *self() noexcept
    {
        static bool_top top;
        return &top;
    }

    // impl value_type

    // unary
    inline value_type const *bnot(value_type const *rhs) const noexcept { return this; }

    // comparators
    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        // TODO: is this correct?
        return (rhs->as<bool_>())
                   ? rhs
                   : new binary_op_not_implemented_type{"==", this, rhs};
    }

    // boolean logic
    inline value_type const *logic_and(value_type const *rhs) const noexcept
    {
        // TODO: is this correct?
        return rhs->as<bool_>()
                   ? rhs
                   : new binary_op_not_implemented_type{"&&", this, rhs};
    }

    inline value_type const *logic_or(value_type const *rhs) const noexcept
    {
        // TODO: is this correct?
        return rhs->as<bool_>()
                   ? rhs
                   : new binary_op_not_implemented_type{"||", this, rhs};
    }
};

struct bool_const final : bool_
{
    inline explicit bool_const(bool b) noexcept
        : b{b} {}

    // impl value_type

    // unary
    inline value_type const *bnot(value_type const *rhs) const noexcept { return new bool_const{!b}; }

    // comparators
    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        if (auto rbool = rhs->as<bool_>())
        {
            // TODO: is this correct?
            if (auto r = rbool->as<bool_const>())
                return new bool_const{b == r->b};
            else
                return r->eq(this);
        }
        else
            return new binary_op_not_implemented_type{"==", this, rhs};
    }

    inline value_type const *logic_and(value_type const *rhs) const noexcept
    {
        if (auto rbool = rhs->as<bool_>())
        {
            // TODO: is this correct?
            if (auto r = rbool->as<bool_const>())
                return new bool_const{b && r->b};
            else
                return r->eq(this);
        }
        return new binary_op_not_implemented_type{"&&", this, rhs};
    }

    inline value_type const *logic_or(value_type const *rhs) const noexcept
    {
        if (auto rbool = rhs->as<bool_>())
        {
            // TODO: is this correct?
            if (auto r = rbool->as<bool_const>())
                return new bool_const{b || r->b};
            else
                return r->eq(this);
        }
        return new binary_op_not_implemented_type{"&&", this, rhs};
    }

    bool b;
};

struct bool_bot final : bool_
{
    inline static value_type const *self() noexcept
    {
        static bool_bot bot;
        return &bot;
    }

    // impl value_type

    // unary
    inline value_type const *bnot(value_type const *rhs) const noexcept { return this; }

    // comparators
    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        return rhs->as<bool_>()
                   ? (value_type const *)this
                   : new binary_op_not_implemented_type{"==", this, rhs};
    }
};