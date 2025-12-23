
#pragma once

#include "types/value.hpp"

// TODO: void cannot be instantiated, so there is no top
struct void_type final : type
{
    // TODO: fix this
    inline value const *top() const noexcept;
    inline value const *zero() const noexcept;

    inline char const *name() const noexcept { return "void"; }
};

struct void_value final : value
{
    inline static value const *self() noexcept
    {
        static void_value vty;
        return &vty;
    }
};

inline value const *void_type::top() const noexcept { return void_value::self(); }
inline value const *void_type::zero() const noexcept { return void_value::self(); }