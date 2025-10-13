
#pragma once

#include "types/type.hpp"

// TODO: this should be a `value_type`
struct ptr final : type
{
    inline static ptr const *not_nil() noexcept
    {
        static ptr ty{};
        return &ty;
    }

    inline static ptr const *nil() noexcept
    {
        static ptr ty{};
        return &ty;
    }

private:
    explicit ptr() noexcept {}
};
