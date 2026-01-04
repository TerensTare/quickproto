
#pragma once

#include "types/value.hpp"

struct rune_type final : type
{
    inline rune_type() noexcept {}

    inline value const *top() const noexcept;
    inline value const *zero() const noexcept;

    // TODO: denote the name better
    inline char const *name() const noexcept { return "rune"; }
};

struct rune : value
{
};

struct rune_top final : rune
{
    inline static value const *self() noexcept
    {
        static rune_top top;
        return &top;
    }

    // TODO: more operators
    inline value const *assign(value const *rhs) const noexcept
    {
        // TODO: implement
        return value::assign(rhs);
    }

    inline value const *phi(value const *other) const noexcept
    {
        return other->as<rune>()
                   ? this
                   : top_value::self();
    }
};

struct rune_bot final : rune
{
    inline static value const *self() noexcept
    {
        static rune_bot bot;
        return &bot;
    }

    // TODO: more operators
    inline value const *assign(value const *rhs) const noexcept
    {
        // TODO: implement
        return value::assign(rhs);
    }

    inline value const *phi(value const *other) const noexcept
    {
        return other->as<rune>()
                   ? other
                   : top_value::self();
    }
};

struct rune_value final : rune
{
    // TODO: `rune` is an alias to `int32`, address that
    inline explicit rune_value(int32_t ch) noexcept {}

    inline value const *phi(value const *other) const noexcept
    {
        auto r = other->as<rune_value>();
        if (!r)
            return top_value::self();

        if (r->as<rune_top>())
            return r;
        if (auto cr = r->as<rune_value>())
            return (ch == cr->ch) ? this : rune_top::self();
        if (r->as<rune_bot>())
            return this;
    }

    int32_t ch;
};

inline value const *rune_type::top() const noexcept { return rune_top::self(); }
// TODO: this should give a zero-rune instead, address this
inline value const *rune_type::zero() const noexcept { return new rune_value{0}; }