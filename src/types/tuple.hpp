
#pragma once

#include <memory>
#include <span>

#include "types/value.hpp"

struct tuple : value_type
{
    // TODO: print concrete type instead
    inline char const *name() const noexcept final { return "tuple"; }
};

struct tuple_top : tuple
{
};

struct tuple_bot : tuple
{
};

struct tuple_n : tuple
{
    inline tuple_n(size_t n, std::unique_ptr<value_type const *[]> sub) noexcept
        : n{n}, sub(std::move(sub)) {}

    // inline type const *meet(type const *rhs) const noexcept
    // {
    //     // TODO: return something better here
    //     if (!rhs)
    //         return nullptr;

    //     if (auto ptr = rhs->as<tuple_n>(); ptr && ptr->n == n)
    //     {
    //         // TODO: is this correct?
    //         auto new_tuple = std::make_unique_for_overwrite<value_type const *[]>(n);
    //         for (size_t i{}; i < n; ++i)
    //         {
    //             new_tuple[i] = sub[i]->meet(ptr->sub[i]);
    //         }

    //         return new tuple_n{n, std::move(new_tuple)};
    //     }

    //     return new tuple_bot{};
    // }

    size_t n;
    std::unique_ptr<value_type const *[]> sub;
};