
#pragma once

#include <memory>
#include <span>

#include "types/value.hpp"

// TODO: make this more descriptive
struct bad_args_count final : value_error
{
    inline bad_args_count(size_t expected, size_t got)
        : expected{expected}, got{got} {}

    size_t expected, got;
};

// TODO: optimize the storage here
struct func final : value
{
    inline func(bool is_extern, value const *ret, smallvec<value const *> params) noexcept
        : is_extern{is_extern}, ret{ret}, params(std::move(params)) {}

    // TODO: you can avoid the span here if you ensure out of this function that `args` is of the correct size
    inline value const *call(std::span<value const *> args) const noexcept
    {
        if (params.n != args.size())
            return new bad_args_count{params.n, args.size()};

        // TODO: typecheck the arguments
        // TODO: how do you denote that this call should be inlined to the optimizer?
        // TODO: implement
        return ret;
    }

    bool is_extern = false;
    value const *ret;
    smallvec<value const *> params;
};

struct func_type final : type
{
    inline func_type(bool is_extern, type const *ret, smallvec<type const *> params)
        : is_extern{is_extern}, ret{ret}, params(std::move(params)) {}

    inline value const *top() const noexcept
    {
        auto const n = params.n;
        auto args = std::make_unique_for_overwrite<value const *[]>(n);
        for (size_t i{}; i < n; ++i)
            args[i] = params[i]->top();

        return new func{is_extern, ret->top(), {n, std::move(args)}};
    }

    inline value const *zero() const noexcept
    {
        auto const n = params.n;
        auto args = std::make_unique_for_overwrite<value const *[]>(n);
        for (size_t i{}; i < n; ++i)
            args[i] = params[i]->zero();

        // TODO: this should be a null pointer instead; address this
        return new func{is_extern, ret->top(), {n, std::move(args)}};
    }

    // TODO: give out the full name of the func
    inline char const *name() const noexcept { return "func"; }

    bool is_extern;
    type const *ret;
    smallvec<type const *> params;
};
