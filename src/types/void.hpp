
#pragma once

#include "value.hpp"

struct void_type final : value_type
{
    inline static value_type const *self() noexcept
    {
        static void_type vty;
        return &vty;
    }

    inline char const *name() const noexcept { return "void"; }
};
