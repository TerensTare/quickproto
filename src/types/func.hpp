
#pragma once

#include <memory>
#include <span>

#include "types/value.hpp"

// TODO: what is the minimal information you know about a function?

// TODO: make this more descriptive
struct bad_args_count final : value_error
{
    inline bad_args_count(size_t expected, size_t got)
        : expected{expected}, got{got} {}

    size_t expected, got;
};

struct func : value
{
};

struct func_top final : func
{
    inline static func_top const *self() noexcept
    {
        static func_top top;
        return &top;
    }

    inline value const *phi(value const *other) const noexcept
    {
        return other->as<func>()
                   ? this
                   : top_value::self();
    }
};

struct func_bot final : func
{
    inline static func_bot const *self() noexcept
    {
        static func_bot bot;
        return &bot;
    }

    inline value const *phi(value const *other) const noexcept
    {
        return other->as<func>()
                   ? other
                   : top_value::self();
    }
};

// TODO: optimize the storage here
struct func_const final : func
{
    inline func_const(bool is_extern, value const *ret, smallvec<value const *> params) noexcept
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

    inline value const *phi(value const *other) const noexcept
    {
        auto f = other->as<func>();
        if (!f)
            return top_value::self();

        if (f->as<func_top>())
            return f;
        // TODO: keep func info
        if (f->as<func_const>())
            return func_top::self();
        if (f->as<func_bot>())
            return this;
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

        return new func_const{is_extern, ret->top(), {n, std::move(args)}};
    }

    inline value const *zero() const noexcept
    {
        auto const n = params.n;
        auto args = std::make_unique_for_overwrite<value const *[]>(n);
        for (size_t i{}; i < n; ++i)
            args[i] = params[i]->zero();

        // TODO: this should be a null pointer instead; address this
        return new func_const{is_extern, ret->top(), {n, std::move(args)}};
    }

    // TODO: give out the full name of the func
    inline char const *name() const noexcept { return "func"; }

    bool is_extern;
    type const *ret;
    smallvec<type const *> params;
};
