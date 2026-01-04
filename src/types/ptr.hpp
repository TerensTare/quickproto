
#pragma once

#include "types/value.hpp"

struct any_pointer : value
{
};

struct pointer_top final : any_pointer
{
    inline explicit pointer_top(type const *ty) noexcept : ty{ty} {}

    inline value const *phi(value const *other) const noexcept
    {
        return other->as<any_pointer>()
                   ? this
                   : top_value::self();
    }

    type const *ty;
};

// TODO: implement other operators eventually
struct nil_value final : any_pointer
{
    inline static nil_value const *self() noexcept
    {
        static nil_value nil;
        return &nil;
    }

    inline value const *phi(value const *other) const noexcept;
};

struct pointer_value final : any_pointer
{
    inline explicit pointer_value(type const *ty) noexcept : ty{ty} {}

    inline value const *phi(value const *other) const noexcept;

    type const *ty;
};

struct pointer_bot final : any_pointer
{
    inline explicit pointer_bot(type const *ty) noexcept : ty{ty} {}

    inline value const *phi(value const *other) const noexcept
    {
        return other->as<any_pointer>()
                   ? other
                   : top_value::self();
    }

    type const *ty;
};

// TODO: implement
struct pointer_type final : type
{
    inline explicit pointer_type(type const *base) noexcept
        : base{base} {}

    // TODO: cache the value
    inline value const *top() const noexcept { return new pointer_value{this}; }
    inline value const *zero() const noexcept { return new nil_value{}; }
    // TODO: also show the base type
    inline char const *name() const noexcept { return "{pointer}"; }

    type const *base;
};

inline ::value const *nil_value::phi(value const *other) const noexcept
{
    auto p = other->as<any_pointer>();
    if (!p)
        return top_value::self();

    if (p->as<nil_value>())
        return this;
    if (auto v = p->as<pointer_value>())
        return new pointer_top{v->ty};
    if (p->as<pointer_bot>())
        return this;
}

inline ::value const *pointer_value::phi(value const *other) const noexcept
{
    auto p = other->as<any_pointer>();
    if (!p)
        return top_value::self();

    if (p->as<nil_value>())
        return new pointer_top{ty};
    // TODO: check matching pointer types
    if (auto v = p->as<pointer_value>())
        return v;
    if (p->as<pointer_bot>())
        return this;
}