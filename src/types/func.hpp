
#pragma once

#include <memory>
#include <span>

#include "types/value.hpp"

// TODO: make this more descriptive
struct bad_args_count final : value_error
{
    inline bad_args_count(size_t expected, size_t got)
        : expected{expected}, got{got} {}

    inline char const *name() const noexcept { return "<bad-args-count>"; }

    size_t expected, got;
};

// TODO: optimize the storage here
struct func final : value
{
    inline func(bool is_extern, value const *ret, size_t n_params, std::unique_ptr<value const *[]> params) noexcept
        : is_extern{is_extern}, ret{ret}, n_params{n_params}, params(std::move(params)) {}

    // TODO: you can avoid the span here if you ensure out of this function that `args` is of the correct size
    inline value const *call(std::span<value const *> args) const noexcept
    {
        if (n_params != args.size())
            return new bad_args_count{n_params, args.size()};

        // TODO: typecheck the arguments
        // TODO: how do you denote that this call should be inlined to the optimizer?
        // TODO: implement
        return ret;
    }

    // TODO: print concrete type instead
    inline char const *name() const noexcept final { return "func"; }

    bool is_extern = false;
    value const *ret;
    size_t n_params;
    std::unique_ptr<value const *[]> params;
};

struct func_type final : type
{
    inline func_type(bool is_extern, type const *ret, size_t n_params, std::unique_ptr<type const *[]> params)
        : is_extern{is_extern}, ret{ret}, n_params{n_params}, params(std::move(params)) {}

    inline value const *top() const noexcept
    {
        auto args = std::make_unique<value const *[]>(n_params);
        for (size_t i{}; i < n_params; ++i)
            args[i] = params[i]->top();

        return new func{is_extern, ret->top(), n_params, std::move(args)};
    }

    bool is_extern;
    type const *ret;
    size_t n_params;
    std::unique_ptr<type const *[]> params;
};
