
#pragma once

#include "types/value.hpp"

struct bool_value : value
{
    inline value const *assign(value const *rhs) const noexcept
    {
        return rhs->as<bool_value>()
                   ? rhs
                   : value::assign(rhs);
    }
};

struct bool_bot final : bool_value
{
    inline static value const *self() noexcept
    {
        static bool_bot bot;
        return &bot;
    }

    // impl value

    // unary
    inline value const *bnot(value const *rhs) const noexcept { return this; }

    // comparators
    inline value const *eq(value const *rhs) const noexcept
    {
        // TODO: is this correct?
        return (rhs->as<bool_value>())
                   ? rhs
                   : value::eq(rhs);
    }

    // boolean logic
    inline value const *logic_and(value const *rhs) const noexcept
    {
        // TODO: is this correct?
        return rhs->as<bool_value>()
                   ? rhs
                   : value::logic_and(rhs);
    }

    inline value const *logic_or(value const *rhs) const noexcept
    {
        // TODO: is this correct?
        return rhs->as<bool_value>()
                   ? rhs
                   : value::logic_or(rhs);
    }
};

struct bool_const final : bool_value
{
    inline static bool_const const *True() noexcept
    {
        static bool_const val{true};
        return &val;
    }

    inline static bool_const const *False() noexcept
    {
        static bool_const val{false};
        return &val;
    }

    inline static bool_const const *make(bool b) noexcept { return b ? True() : False(); }

    inline bool is_const() const { return true; }

    // impl value

    // unary
    inline value const *bnot() const noexcept { return new bool_const{!b}; }

    // comparators
    inline value const *eq(value const *rhs) const noexcept
    {
        if (auto rbool = rhs->as<bool_value>())
        {
            // TODO: is this correct?
            if (auto r = rbool->as<bool_const>())
                return new bool_const{b == r->b};
            else
                return r->eq(this);
        }
        else
            return value::eq(rhs);
    }

    inline value const *logic_and(value const *rhs) const noexcept
    {
        if (auto rbool = rhs->as<bool_value>())
        {
            // TODO: is this correct?
            if (auto r = rbool->as<bool_const>())
                return new bool_const{b && r->b};
            else
                return r->eq(this);
        }
        return value::logic_and(rhs);
    }

    inline value const *logic_or(value const *rhs) const noexcept
    {
        if (auto rbool = rhs->as<bool_value>())
        {
            // TODO: is this correct?
            if (auto r = rbool->as<bool_const>())
                return new bool_const{b || r->b};
            else
                return r->eq(this);
        }
        return value::logic_or(rhs);
    }

    bool b;

private:
    inline explicit bool_const(bool b) noexcept
        : b{b} {}
};

struct bool_top final : bool_value
{
    inline static value const *self() noexcept
    {
        static bool_top top;
        return &top;
    }

    // impl value

    // logic
    // TODO: is this correct?
    inline value const *logic_and(value const *rhs) const noexcept
    {
        return rhs->as<bool_value>()
                   ? rhs
                   : value::logic_and(rhs);
    }

    inline value const *logic_or(value const *rhs) const noexcept
    {
        return rhs->as<bool_value>()
                   ? rhs
                   : value::logic_or(rhs);
    }

    // unary
    inline value const *bnot() const noexcept { return this; }

    // comparators
    inline value const *eq(value const *rhs) const noexcept
    {
        return rhs->as<bool_value>()
                   ? (value const *)this
                   : value::eq(rhs);
    }
};

struct bool_type final : type
{
    inline value const *top() const noexcept { return bool_top::self(); }
    inline value const *zero() const noexcept { return bool_const::False(); }

    inline char const *name() const noexcept { return "bool"; }
};