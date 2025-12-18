
#pragma once

#include <cstdint>
#include <entt/container/dense_map.hpp>

#include "types/float.hpp"

// HACK: do something better
struct invalid_cast final : value_error
{
    inline invalid_cast(value const *val, type const *target) noexcept : val{val}, target{target} {}

    inline char const *name() const noexcept { return "<invalid-cast>"; }

    value const *val;
    type const *target;
};

struct int_value : value
{
    inline value const *assign(value const *rhs) const noexcept
    {
        return rhs->as<int_value>()
                   ? rhs
                   : value::assign(rhs);
    }

    inline char const *name() const noexcept final { return "int"; }
};

struct int_top final : int_value
{
    inline static value const *self() noexcept
    {
        static int_top top;
        return &top;
    }

    // impl value

    inline value const *add(value const *rhs) const noexcept
    {
        return (rhs->as<int_value>())
                   ? rhs
                   : value::add(rhs);
    }

    inline value const *sub(value const *rhs) const noexcept
    {
        return (rhs->as<int_value>())
                   ? rhs
                   : value::sub(rhs);
    }

    inline value const *mul(value const *rhs) const noexcept
    {
        return (rhs->as<int_value>())
                   ? rhs
                   : value::mul(rhs);
    }

    inline value const *div(value const *rhs) const noexcept
    {
        return (rhs->as<int_value>())
                   ? rhs
                   : value::div(rhs);
    }

    inline value const *neg() const noexcept { return this; }

    inline value const *eq(value const *rhs) const noexcept
    {
        return (rhs->as<int_value>())
                   ? bool_top::self()
                   : value::eq(rhs);
    }

    inline value const *ne(value const *rhs) const noexcept
    {
        return (rhs->as<int_value>())
                   ? bool_top::self()
                   : value::ne(rhs);
    }

    inline value const *lt(value const *rhs) const noexcept
    {
        return (rhs->as<int_value>())
                   ? bool_top::self()
                   : value::lt(rhs);
    }

    inline value const *le(value const *rhs) const noexcept
    {
        return (rhs->as<int_value>())
                   ? bool_top::self()
                   : value::le(rhs);
    }

    inline value const *gt(value const *rhs) const noexcept
    {
        return (rhs->as<int_value>())
                   ? bool_top::self()
                   : value::gt(rhs);
    }

    inline value const *ge(value const *rhs) const noexcept
    {
        return (rhs->as<int_value>())
                   ? bool_top::self()
                   : value::ge(rhs);
    }
};

struct int_bot final : int_value
{
    inline static value const *self() noexcept
    {
        static int_bot bot;
        return &bot;
    }

    // impl value

    inline value const *add(value const *rhs) const noexcept
    {
        return rhs->as<int_value>()
                   ? (value const *)this
                   : value::add(rhs);
    }

    inline value const *sub(value const *rhs) const noexcept
    {
        return rhs->as<int_value>()
                   ? (value const *)this
                   : value::sub(rhs);
    }

    inline value const *mul(value const *rhs) const noexcept
    {
        return rhs->as<int_value>()
                   ? (value const *)this
                   : value::mul(rhs);
    }

    inline value const *div(value const *rhs) const noexcept
    {
        return rhs->as<int_value>()
                   ? (value const *)this
                   : value::div(rhs);
    }

    inline value const *neg() const noexcept { return this; }

    inline value const *eq(value const *rhs) const noexcept
    {
        return rhs->as<int_value>()
                   ? (value const *)this
                   : value::eq(rhs);
    }

    inline value const *ne(value const *rhs) const noexcept
    {
        return rhs->as<int_value>()
                   ? (value const *)this
                   : value::ne(rhs);
    }

    inline value const *lt(value const *rhs) const noexcept
    {
        return rhs->as<int_value>()
                   ? (value const *)this
                   : value::lt(rhs);
    }

    inline value const *le(value const *rhs) const noexcept
    {
        return rhs->as<int_value>()
                   ? (value const *)this
                   : value::le(rhs);
    }

    inline value const *gt(value const *rhs) const noexcept
    {
        return rhs->as<int_value>()
                   ? (value const *)this
                   : value::gt(rhs);
    }

    inline value const *ge(value const *rhs) const noexcept
    {
        return rhs->as<int_value>()
                   ? (value const *)this
                   : value::ge(rhs);
    }

    inline value const *band(value const *rhs) const noexcept
    {
        return rhs->as<int_value>()
                   ? (value const *)this
                   : value::band(rhs);
    }

    inline value const *bxor(value const *rhs) const noexcept
    {
        return rhs->as<int_value>()
                   ? (value const *)this
                   : value::bxor(rhs);
    }

    inline value const *bor(value const *rhs) const noexcept
    {
        return rhs->as<int_value>()
                   ? (value const *)this
                   : value::bor(rhs);
    }
};

struct sint_type final : type
{
    // TODO: address the naming here
    inline value const *top() const noexcept { return int_bot::self(); }
};

struct int_const final : int_value
{
    inline static int_const const *make(int64_t n) noexcept
    {
        // TODO: use something more optimal, remove caching once you get "in-place" types
        static entt::dense_map<int64_t, std::unique_ptr<int_const const>, std::identity> cache;
        if (auto iter = cache.find(n); iter != cache.end())
            return iter->second.get();
        return cache.insert({n, std::unique_ptr<int_const>(new int_const{n})}).first->second.get();
    }

    inline bool is_const() const { return true; }

    // impl value
    inline value const *add(value const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return int_const::make(n + ptr->n);
        else if (rhs->as<int_top>())
            return this;
        else
            return value::add(rhs);
    }

    inline value const *sub(value const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return int_const::make(n - ptr->n);
        else if (rhs->as<int_top>())
            return this;
        else
            return value::sub(rhs);
    }

    inline value const *mul(value const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return int_const::make(n * ptr->n);
        else if (rhs->as<int_top>())
            return this;
        else
            return value::mul(rhs);
    }

    inline value const *div(value const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return (ptr->n == 0) ? int_top::self() : int_const::make(n / ptr->n);
        else if (rhs->as<int_top>())
            return this;
        else
            return value::div(rhs);
    }

    inline value const *neg() const noexcept { return int_const::make(-n); }

    inline value const *eq(value const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return bool_const::make(n == ptr->n);
        else if (rhs->as<int_top>())
            return bool_top::self();
        else
            return value::eq(rhs);
    }

    inline value const *ne(value const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return bool_const::make(n != ptr->n);
        else if (rhs->as<int_top>())
            return bool_top::self();
        else
            return value::ne(rhs);
    }

    inline value const *lt(value const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return bool_const::make(n < ptr->n);
        else if (rhs->as<int_top>())
            return bool_top::self();
        else
            return value::lt(rhs);
    }

    inline value const *le(value const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return bool_const::make(n <= ptr->n);
        else if (rhs->as<int_top>())
            return bool_top::self();
        else
            return value::le(rhs);
    }

    inline value const *gt(value const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return bool_const::make(n > ptr->n);
        else if (rhs->as<int_top>())
            return bool_top::self();
        else
            return value::gt(rhs);
    }

    inline value const *ge(value const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return bool_const::make(n >= ptr->n);
        else if (rhs->as<int_top>())
            return bool_top::self();
        else
            return value::ge(rhs);
    }

    inline value const *band(value const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return int_const::make(n & ptr->n);
        else if (rhs->as<int_top>())
            return this;
        else
            return value::band(rhs);
    }

    inline value const *bxor(value const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return int_const::make(n ^ ptr->n);
        else if (rhs->as<int_top>())
            return this;
        else
            return value::bxor(rhs);
    }

    inline value const *bor(value const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return int_const::make(n | ptr->n);
        else if (rhs->as<int_top>())
            return this;
        else
            return value::bor(rhs);
    }

    inline value const *cast(type const *target) const noexcept
    {
        if (target->as<float64_type>())
            return new float64{(double)n};
        else if (target->as<sint_type>())
            return this;
        else
            return new invalid_cast{this, target};
    }

    int64_t n;

private:
    explicit int_const(int64_t n) noexcept : n{n} {}
};

// TODO: two or more possible ranges (eg. [-5, -3] to [3, 5])
// TODO: lt, gt, le, ge
struct int_range final : int_value
{
    inline int_range(int64_t lo = INT64_MIN, int64_t hi = INT64_MAX) noexcept
        : lo{lo}, hi{hi} {}

    int64_t lo, hi;
};
