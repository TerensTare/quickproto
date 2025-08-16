
#pragma once

#include <concepts>
#include <cstdint>
#include <vector>

// TODO:
// - what is the type of an error?
// - use a custom typeid solution, rather than dynamic cast
// ^ How do you represent subclassing with indices? (eg. how do you know a type is any kind of `int`?)
// ^ but do you really need subclassing?

struct type
{
    virtual ~type() {}

    virtual type const *meet(type const *rhs) const noexcept = 0;

    // TODO: ensure derived from type
    template <typename T>
    inline T const *as() const noexcept { return dynamic_cast<T const *>(this); }
};

struct top final : type
{
    inline type const *meet(type const *rhs) const noexcept { return rhs; }
};

struct bot final : type
{
    inline static type const *self() noexcept
    {
        static bot bot;
        return &bot;
    }

    inline type const *meet(type const *) const noexcept { return this; }
};

struct not_ctrl;
struct int_;

// reachable node
struct ctrl final : type
{
    inline type const *meet(type const *rhs) const noexcept
    {
        if (rhs->as<bot>())
            return rhs;
        else if (rhs->as<ctrl>() || rhs->as<not_ctrl>() || rhs->as<top>())
            return this;
        else if (rhs->as<int_>())
            return bot::self();
    }
};

// unreachable node
struct not_ctrl final : type
{
    inline type const *meet(type const *rhs) const noexcept
    {
        if (rhs->as<bot>() || rhs->as<ctrl>() || rhs->as<not_ctrl>())
            return rhs;
        else if (rhs->as<top>())
            return this;
        else if (rhs->as<int_>())
            return bot::self();
    }
};

struct int_ : type
{
};

struct int_top final : int_
{
    inline type const *meet(type const *rhs) const noexcept
    {
        if (rhs->as<int_>())
            return rhs;
        // TODO: handle non-int meets
    }
};

struct int_bot final : int_
{
    inline static type const *self() noexcept
    {
        static int_bot bot;
        return &bot;
    }

    inline type const *meet(type const *rhs) const noexcept { return this; }
};

struct int_const final : int_
{
    explicit int_const(int64_t n) noexcept : n{n} {}

    inline type const *meet(type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return (ptr->n == n) ? this : int_bot::self();
        else if (rhs->as<int_top>())
            return this;
    }

    int64_t n;
};

struct tuple : type
{
};
struct tuple_top : tuple
{
};
struct tuple_n : tuple
{
};
struct tuple_bot : tuple
{
};
