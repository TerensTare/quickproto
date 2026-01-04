
#pragma once

#include "types/value.hpp"

struct string_type final : type
{
    inline string_type() noexcept {}

    inline value const *top() const noexcept;
    inline value const *zero() const noexcept;

    // TODO: denote the name better
    inline char const *name() const noexcept { return "string"; }
};

struct string_ : value
{
};

// TODO: str + str
struct string_top final : string_
{
    inline static value const *self() noexcept
    {
        static string_top top;
        return &top;
    }

    inline value const *assign(value const *rhs) const noexcept
    {
        // TODO: implement
        return value::assign(rhs);
    }

    inline value const *phi(value const *other) const noexcept
    {
        return other->as<string_>()
                   ? this
                   : top_value::self();
    }
};

struct string_bot final : string_
{
    inline value const *phi(value const *other) const noexcept
    {
        return other->as<string_>()
                   ? other
                   : top_value::self();
    }
};

// TODO: keep string info here
struct string_value final : string_
{
    inline string_value() noexcept {}

    inline value const *phi(value const *other) const noexcept
    {
        auto s = other->as<string_>();
        if (!s)
            return top_value::self();

        if (s->as<string_top>())
            return s;
        // TODO: keep string constant info
        if (auto cs = s->as<string_value>())
            return string_top::self();
        if (s->as<string_bot>())
            return this;
    }
};


inline value const *string_type::top() const noexcept { return string_top::self(); }
// TODO: this should give a zero-string instead, address this
inline value const *string_type::zero() const noexcept { return new string_value; }