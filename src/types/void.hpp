
#pragma once

#include "types/value.hpp"

// TODO: void cannot be instantiated, so there is no top
struct void_type final : type
{
    // TODO: fix this
    inline value const *top() const noexcept;
};

struct void_value final : value
{
    inline static value const *self() noexcept
    {
        static void_value vty;
        return &vty;
    }

    inline char const *name() const noexcept { return "void"; }
};

inline value const *void_type::top() const noexcept { return void_value::self(); }