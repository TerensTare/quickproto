
#pragma once

#include "types/bool.hpp"

struct float_ : value_type
{
    // TODO: recheck this
    inline value_type const *assign(value_type const *rhs) const noexcept
    {
        return rhs->as<float_>()
                   ? rhs
                   : value_type::assign(rhs);
    }

    inline char const *name() const noexcept override { return "{float}"; }
};

struct float_top final : float_
{
    inline static value_type const *self() noexcept
    {
        static float_top top;
        return &top;
    }

    // impl value_type

    inline value_type const *add(value_type const *rhs) const noexcept
    {
        return (rhs->as<float_>())
                   ? rhs
                   : new binary_op_not_implemented_type{"+", this, rhs};
    }

    inline value_type const *sub(value_type const *rhs) const noexcept
    {
        return (rhs->as<float_>())
                   ? rhs
                   : new binary_op_not_implemented_type{"-", this, rhs};
    }

    inline value_type const *mul(value_type const *rhs) const noexcept
    {
        return (rhs->as<float_>())
                   ? rhs
                   : new binary_op_not_implemented_type{"*", this, rhs};
    }

    inline value_type const *div(value_type const *rhs) const noexcept
    {
        return (rhs->as<float_>())
                   ? rhs
                   : new binary_op_not_implemented_type{"/", this, rhs};
    }

    inline value_type const *neg() const noexcept { return this; }

    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        return (rhs->as<float_>())
                   ? bool_top::self()
                   : new binary_op_not_implemented_type{"==", this, rhs};
    }

    inline value_type const *lt(value_type const *rhs) const noexcept
    {
        return (rhs->as<float_>())
                   ? bool_top::self()
                   : new binary_op_not_implemented_type{"<", this, rhs};
    }
};

struct float_bot final : float_
{
    inline static value_type const *self() noexcept
    {
        static float_bot bot;
        return &bot;
    }

    // impl value_type

    // TODO: fail if type is NOT `float_`
    inline value_type const *add(value_type const *rhs) const noexcept { return this; }
    inline value_type const *sub(value_type const *rhs) const noexcept { return this; }
    inline value_type const *mul(value_type const *rhs) const noexcept { return this; }
    inline value_type const *div(value_type const *rhs) const noexcept { return this; }

    inline value_type const *neg() const noexcept { return this; }

    inline value_type const *eq(value_type const *rhs) const noexcept { return this; }
    inline value_type const *lt(value_type const *rhs) const noexcept { return this; }
};

struct float32 final : float_
{
    explicit float32(float f) noexcept : f{f} {}

    // impl value_type
    inline value_type const *add(value_type const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float32>())
            return new float32{f + ptr->f};
        else if (rhs->as<float_top>())
            return this;
        else
            return new binary_op_not_implemented_type{"+", this, rhs};
    }

    inline value_type const *sub(value_type const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float32>())
            return new float32{f - ptr->f};
        else if (rhs->as<float_top>())
            return this;
        else
            return new binary_op_not_implemented_type{"-", this, rhs};
    }

    inline value_type const *mul(value_type const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float32>(); ptr)
            return new float32{f * ptr->f};
        else if (rhs->as<float_top>())
            return this;
        else
            return new binary_op_not_implemented_type{"*", this, rhs};
    }

    inline value_type const *div(value_type const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float32>(); ptr)
            return (ptr->f == 0.0f) ? float_top::self() : new float32{f / ptr->f};
        else if (rhs->as<float_top>())
            return this;
        else
            return new binary_op_not_implemented_type{"/", this, rhs};
    }

    inline value_type const *neg() const noexcept { return new float32{-f}; }

    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<float32>(); ptr)
            return new bool_const{f == ptr->f};
        else if (rhs->as<float_top>())
            return bool_top::self();
        else
            return new binary_op_not_implemented_type{"==", this, rhs};
    }

    inline value_type const *lt(value_type const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<float32>(); ptr)
            return new bool_const{f < ptr->f};
        else if (rhs->as<float_top>())
            return bool_top::self();
        else
            return new binary_op_not_implemented_type{"<", this, rhs};
    }

    inline char const *name() const noexcept final { return "float32"; }

    float f;
};

struct float64 final : float_
{
    explicit float64(double d) noexcept : d{d} {}

    // impl value_type
    inline value_type const *add(value_type const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float64>())
            return new float64{d + ptr->d};
        else if (rhs->as<float_top>())
            return this;
        else
            return new binary_op_not_implemented_type{"+", this, rhs};
    }

    inline value_type const *sub(value_type const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float64>())
            return new float64{d - ptr->d};
        else if (rhs->as<float_top>())
            return this;
        else
            return new binary_op_not_implemented_type{"-", this, rhs};
    }

    inline value_type const *mul(value_type const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float64>(); ptr)
            return new float64{d * ptr->d};
        else if (rhs->as<float_top>())
            return this;
        else
            return new binary_op_not_implemented_type{"*", this, rhs};
    }

    inline value_type const *div(value_type const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return rhs;
        else if (auto ptr = rhs->as<float64>(); ptr)
            return (ptr->d == 0.0) ? float_top::self() : new float64{d / ptr->d};
        else if (rhs->as<float_top>())
            return this;
        else
            return new binary_op_not_implemented_type{"/", this, rhs};
    }

    inline value_type const *neg() const noexcept { return new float64{-d}; }

    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<float64>(); ptr)
            return new bool_const{d == ptr->d};
        else if (rhs->as<float_top>())
            return bool_top::self();
        else
            return new binary_op_not_implemented_type{"==", this, rhs};
    }

    inline value_type const *lt(value_type const *rhs) const noexcept
    {
        if (rhs->as<float_bot>())
            return bool_bot::self();
        else if (auto ptr = rhs->as<float64>(); ptr)
            return new bool_const{d < ptr->d};
        else if (rhs->as<float_top>())
            return bool_top::self();
        else
            return new binary_op_not_implemented_type{"<", this, rhs};
    }

    inline char const *name() const noexcept final { return "float64"; }

    double d;
};