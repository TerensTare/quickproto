
#pragma once

#include <cstdint>
#include <entt/container/dense_map.hpp>
#include "types/bool.hpp"

struct int_ : value_type
{
    inline value_type const *assign(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? rhs
                   : value_type::assign(rhs);
    }

    inline char const *name() const noexcept final { return "int"; }
};

struct int_top final : int_
{
    inline static value_type const *self() noexcept
    {
        static int_top top;
        return &top;
    }

    // impl value_type

    inline value_type const *add(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? rhs
                   : value_type::add(rhs);
    }

    inline value_type const *sub(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? rhs
                   : value_type::sub(rhs);
    }

    inline value_type const *mul(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? rhs
                   : value_type::mul(rhs);
    }

    inline value_type const *div(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? rhs
                   : value_type::div(rhs);
    }

    inline value_type const *neg() const noexcept { return this; }

    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? bool_top::self()
                   : value_type::eq(rhs);
    }

    inline value_type const *ne(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? bool_top::self()
                   : value_type::ne(rhs);
    }

    inline value_type const *lt(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? bool_top::self()
                   : value_type::lt(rhs);
    }

    inline value_type const *le(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? bool_top::self()
                   : value_type::le(rhs);
    }

    inline value_type const *gt(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? bool_top::self()
                   : value_type::gt(rhs);
    }

    inline value_type const *ge(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? bool_top::self()
                   : value_type::ge(rhs);
    }
};

struct int_bot final : int_
{
    inline static value_type const *self() noexcept
    {
        static int_bot bot;
        return &bot;
    }

    // impl value_type

    inline value_type const *add(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : value_type::add(rhs);
    }

    inline value_type const *sub(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : value_type::sub(rhs);
    }

    inline value_type const *mul(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : value_type::mul(rhs);
    }

    inline value_type const *div(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : value_type::div(rhs);
    }

    inline value_type const *neg() const noexcept { return this; }

    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : value_type::eq(rhs);
    }

    inline value_type const *ne(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : value_type::ne(rhs);
    }

    inline value_type const *lt(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : value_type::lt(rhs);
    }

    inline value_type const *le(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : value_type::le(rhs);
    }

    inline value_type const *gt(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : value_type::gt(rhs);
    }

    inline value_type const *ge(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : value_type::ge(rhs);
    }

    inline value_type const *band(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : value_type::band(rhs);
    }

    inline value_type const *bxor(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : value_type::bxor(rhs);
    }

    inline value_type const *bor(value_type const *rhs) const noexcept
    {
        return rhs->as<int_>()
                   ? (value_type const *)this
                   : value_type::bor(rhs);
    }
};

struct int_const final : int_
{
    inline static int_const const *value(int64_t n) noexcept
    {
        // TODO: use something more optimal, remove caching once you get "in-place" types
        static entt::dense_map<int64_t, std::unique_ptr<int_const const>, std::identity> cache;
        if (auto iter = cache.find(n); iter != cache.end())
            return iter->second.get();
        return cache.insert({n, std::unique_ptr<int_const>(new int_const{n})}).first->second.get();
    }

    inline bool is_const() const { return true; }

    // impl value_type
    inline value_type const *add(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new int_const{n + ptr->n};
        else if (rhs->as<int_top>())
            return this;
        else
            return value_type::add(rhs);
    }

    inline value_type const *sub(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new int_const{n - ptr->n};
        else if (rhs->as<int_top>())
            return this;
        else
            return value_type::sub(rhs);
    }

    inline value_type const *mul(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new int_const{n * ptr->n};
        else if (rhs->as<int_top>())
            return this;
        else
            return value_type::mul(rhs);
    }

    inline value_type const *div(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return (ptr->n == 0) ? int_top::self() : new int_const{n / ptr->n};
        else if (rhs->as<int_top>())
            return this;
        else
            return value_type::div(rhs);
    }

    inline value_type const *neg() const noexcept { return new int_const{-n}; }

    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new bool_const{n == ptr->n};
        else if (rhs->as<int_top>())
            return bool_top::self();
        else
            return value_type::eq(rhs);
    }

    inline value_type const *ne(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new bool_const{n != ptr->n};
        else if (rhs->as<int_top>())
            return bool_top::self();
        else
            return value_type::ne(rhs);
    }

    inline value_type const *lt(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new bool_const{n < ptr->n};
        else if (rhs->as<int_top>())
            return bool_top::self();
        else
            return value_type::lt(rhs);
    }

    inline value_type const *le(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new bool_const{n <= ptr->n};
        else if (rhs->as<int_top>())
            return bool_top::self();
        else
            return value_type::le(rhs);
    }

    inline value_type const *gt(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new bool_const{n > ptr->n};
        else if (rhs->as<int_top>())
            return bool_top::self();
        else
            return value_type::gt(rhs);
    }

    inline value_type const *ge(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new bool_const{n >= ptr->n};
        else if (rhs->as<int_top>())
            return bool_top::self();
        else
            return value_type::ge(rhs);
    }

    inline value_type const *band(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new int_const{n & ptr->n};
        else if (rhs->as<int_top>())
            return this;
        else
            return value_type::band(rhs);
    }

    inline value_type const *bxor(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new int_const{n ^ ptr->n};
        else if (rhs->as<int_top>())
            return this;
        else
            return value_type::bxor(rhs);
    }

    inline value_type const *bor(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new int_const{n | ptr->n};
        else if (rhs->as<int_top>())
            return this;
        else
            return value_type::bor(rhs);
    }

    int64_t n;

private:
    explicit int_const(int64_t n) noexcept : n{n} {}
};

// TODO: two or more possible ranges (eg. [-5, -3] to [3, 5])
// TODO: lt, gt, le, ge
struct int_range final : int_
{
    inline int_range(int64_t lo = INT64_MIN, int64_t hi = INT64_MAX) noexcept
        : lo{lo}, hi{hi} {}

    int64_t lo, hi;
};
