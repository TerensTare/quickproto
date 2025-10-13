
#pragma once

#include <memory>
#include <span>

#include "types/value.hpp"

struct func final : value_type
{
    inline func(value_type const *ret, size_t n_params, std::unique_ptr<value_type const *[]> params) noexcept
        : ret{ret}, n_params{n_params}, params(std::move(params)) {}

    // TODO: you can avoid the span here if you ensure out of this function that `args` is of the correct size
    inline value_type const *call(std::span<value_type const *> args) const noexcept
    {
        // TODO: implement
        // TODO: typecheck the arguments
        return ret;
    }

    // TODO: print concrete type instead
    inline char const *name() const noexcept final { return "func"; }

    value_type const *ret;
    size_t n_params;
    std::unique_ptr<value_type const *[]> params;
};
