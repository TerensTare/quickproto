
#pragma once

#include "types/bool.hpp"

struct float_value : value
{
    // TODO: recheck this
    inline value const *assign(value const *rhs) const noexcept
    {
        return rhs->as<float_value>()
                   ? rhs
                   : value::assign(rhs);
    }

    inline char const *name() const noexcept override { return "{float}"; }
};

struct float_top final : float_value
{
    inline static value const *self() noexcept
    {
        static float_top top;
        return &top;
    }

    // impl value

    inline value const *add(value const *rhs) const noexcept
    {
        return (rhs->as<float_value>())
                   ? rhs
                   : value::add(rhs);
    }

    inline value const *sub(value const *rhs) const noexcept
    {
        return (rhs->as<float_value>())
                   ? rhs
                   : value::sub(rhs);
    }

    inline value const *mul(value const *rhs) const noexcept
    {
        return (rhs->as<float_value>())
                   ? rhs
                   : value::mul(rhs);
    }

    inline value const *div(value const *rhs) const noexcept
    {
        return (rhs->as<float_value>())
                   ? rhs
                   : value::div(rhs);
    }

    inline value const *neg() const noexcept { return this; }

    inline value const *eq(value const *rhs) const noexcept
    {
        return (rhs->as<float_value>())
                   ? bool_top::self()
                   : value::eq(rhs);
    }

    inline value const *lt(value const *rhs) const noexcept
    {
        return (rhs->as<float_value>())
                   ? bool_top::self()
                   : value::lt(rhs);
    }
};

struct float_bot final : float_value
{
    inline static value const *self() noexcept
    {
        static float_bot bot;
        return &bot;
    }

    // impl value

    // TODO: fail if type is NOT `float_value`
    inline value const *add(value const *rhs) const noexcept { return this; }
    inline value const *sub(value const *rhs) const noexcept { return this; }
    inline value const *mul(value const *rhs) const noexcept { return this; }
    inline value const *div(value const *rhs) const noexcept { return this; }

    inline value const *neg() const noexcept { return this; }

    inline value const *eq(value const *rhs) const noexcept { return this; }
    inline value const *lt(value const *rhs) const noexcept { return this; }
};

struct float32 final : float_value
{
    explicit float32(float f) noexcept : f{f} {}

    inline bool is_const() const { return true; }

    // impl value
    inline value const *add(value const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float32>())
            return new float32{f + ptr->f};
        else if (rhs->as<float_top>())
            return this;
        else
            return value::add(rhs);
    }

    inline value const *sub(value const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float32>())
            return new float32{f - ptr->f};
        else if (rhs->as<float_top>())
            return this;
        else
            return value::sub(rhs);
    }

    inline value const *mul(value const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float32>(); ptr)
            return new float32{f * ptr->f};
        else if (rhs->as<float_top>())
            return this;
        else
            return value::mul(rhs);
    }

    inline value const *div(value const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float32>(); ptr)
            return (ptr->f == 0.0f) ? float_top::self() : new float32{f / ptr->f};
        else if (rhs->as<float_top>())
            return this;
        else
            return value::div(rhs);
    }

    inline value const *neg() const noexcept { return new float32{-f}; }

    inline value const *eq(value const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<float32>(); ptr)
            return new bool_const{f == ptr->f};
        else if (rhs->as<float_top>())
            return bool_top::self();
        else
            return value::eq(rhs);
    }

    inline value const *lt(value const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<float32>(); ptr)
            return new bool_const{f < ptr->f};
        else if (rhs->as<float_top>())
            return bool_top::self();
        else
            return value::lt(rhs);
    }

    inline char const *name() const noexcept final { return "float32"; }

    float f;
};

struct float64 final : float_value
{
    explicit float64(double d) noexcept : d{d} {}

    inline bool is_const() const { return true; }

    // impl value
    inline value const *add(value const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float64>())
            return new float64{d + ptr->d};
        else if (rhs->as<float_top>())
            return this;
        else
            return value::add(rhs);
    }

    inline value const *sub(value const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float64>())
            return new float64{d - ptr->d};
        else if (rhs->as<float_top>())
            return this;
        else
            return value::sub(rhs);
    }

    inline value const *mul(value const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float64>(); ptr)
            return new float64{d * ptr->d};
        else if (rhs->as<float_top>())
            return this;
        else
            return value::mul(rhs);
    }

    inline value const *div(value const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float64>(); ptr)
            return (ptr->d == 0.0) ? float_top::self() : new float64{d / ptr->d};
        else if (rhs->as<float_top>())
            return this;
        else
            return value::div(rhs);
    }

    inline value const *neg() const noexcept { return new float64{-d}; }

    inline value const *eq(value const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<float64>(); ptr)
            return new bool_const{d == ptr->d};
        else if (rhs->as<float_top>())
            return bool_top::self();
        else
            return value::eq(rhs);
    }

    inline value const *lt(value const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<float64>(); ptr)
            return new bool_const{d < ptr->d};
        else if (rhs->as<float_top>())
            return bool_top::self();
        else
            return value::lt(rhs);
    }

    inline char const *name() const noexcept final { return "float64"; }

    double d;
};

struct float64_type final : type
{
    // TODO: address the naming here
    inline value const *top() const noexcept { return float_bot::self(); }
};