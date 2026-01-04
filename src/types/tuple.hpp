
#pragma once

#include <memory>
#include <span>

#include "types/value.hpp"

struct tuple : value
{
};

struct tuple_top : tuple
{
    inline static tuple_top const *self() noexcept
    {
        static tuple_top top;
        return &top;
    }

    inline value const *phi(value const *other) const noexcept
    {
        return other->as<tuple>()
                   ? this
                   : top_value::self();
    }
};

struct tuple_bot : tuple
{
    inline static tuple_bot const *self() noexcept
    {
        static tuple_bot bot;
        return &bot;
    }

    inline value const *phi(value const *other) const noexcept
    {
        return other->as<string_value>()
                   ? other
                   : top_value::self();
    }
};

struct tuple_n : tuple
{
    inline tuple_n(size_t n, std::unique_ptr<value const *[]> sub) noexcept
        : n{n}, sub(std::move(sub)) {}

    // inline type const *meet(type const *rhs) const noexcept
    // {
    //     // TODO: return something better here
    //     if (!rhs)
    //         return nullptr;

    //     if (auto ptr = rhs->as<tuple_n>(); ptr && ptr->n == n)
    //     {
    //         // TODO: is this correct?
    //         auto new_tuple = std::make_unique_for_overwrite<value const *[]>(n);
    //         for (size_t i{}; i < n; ++i)
    //         {
    //             new_tuple[i] = sub[i]->meet(ptr->sub[i]);
    //         }

    //         return new tuple_n{n, std::move(new_tuple)};
    //     }

    //     return new tuple_bot{};
    // }

    inline value const *phi(value const *other) const noexcept
    {
        auto t = other->as<tuple>();
        if (!t)
            return top_value::self();

        if (t->as<tuple_top>())
            return t;
        // TODO: preserve info still
        if (t->as<tuple_n>())
            return tuple_top::self();
        if (t->as<tuple_bot>())
            return this;
    }

    size_t n;
    std::unique_ptr<value const *[]> sub;
};