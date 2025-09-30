
#pragma once

#include <concepts>
#include <cstdint>
#include <vector>

// TODO:
// - what is the type of an error?
// - use a custom typeid solution, rather than dynamic cast
// ^ How do you represent subclassing with indices? (eg. how do you know a type is any kind of `int`?)
// ^ but do you really need subclassing?
// - separate value from control flow nodes
// ^ value and control flow nodes have disjoint types, so make separate types somehow
// - signed and unsigned integers should be separate; then you only have these types for integers
// ^ then sized integers are for example: uint8 is `uint_range{0, 255}`
// - maybe use a value_stack and index into it when representing types
// - `control_flow_type`
// - maybe handle operators using a function table, rather than letting the type handle them?
// - get rid of `meet` eventually; replace for specific operations

struct type
{
    virtual ~type() {}

    virtual type const *meet(type const *rhs) const noexcept = 0;

    // TODO: T: type
    template <typename T>
    inline T const *as() const noexcept { return dynamic_cast<T const *>(this); }
};

// impossible type (unreachable code, UB, errors, etc.)
struct top final : type
{
    inline static type const *self() noexcept
    {
        static top top;
        return &top;
    }

    inline type const *meet(type const *rhs) const noexcept { return rhs; }
};

// unknown type (runtime values, etc.)
struct bot final : type
{
    inline static type const *self() noexcept
    {
        static bot bot;
        return &bot;
    }

    inline type const *meet(type const *) const noexcept { return this; }
};

struct not_ctrl;
struct int_;

// Section: control flow types

// reachable node
struct ctrl final : type
{
    inline type const *meet(type const *rhs) const noexcept
    {
        if (rhs->as<bot>())
            return rhs;
        else if (rhs->as<ctrl>() || rhs->as<not_ctrl>() || rhs->as<top>())
            return this;
        else if (rhs->as<int_>())
            return bot::self();
    }
};

// unreachable node
struct not_ctrl final : type
{
    inline type const *meet(type const *rhs) const noexcept
    {
        if (rhs->as<bot>() || rhs->as<ctrl>() || rhs->as<not_ctrl>())
            return rhs;
        else if (rhs->as<top>())
            return this;
        else if (rhs->as<int_>())
            return bot::self();
    }
};

struct value_type : type
{
    // binary
    virtual value_type const *add(value_type const *rhs) const noexcept = 0;
    virtual value_type const *sub(value_type const *rhs) const noexcept = 0;
    virtual value_type const *mul(value_type const *rhs) const noexcept = 0;
    virtual value_type const *div(value_type const *rhs) const noexcept = 0;
    // unary
    virtual value_type const *neg() const noexcept = 0;
};

// signals that this operator/overload is not implemented
struct op_not_implemented_type final : value_type
{
    inline static value_type const *self() noexcept
    {
        static op_not_implemented_type ty;
        return &ty;
    }

    inline type const *meet(type const *rhs) const noexcept { return op_not_implemented_type::self(); }

    // TODO: return a `value_error` instead
    inline value_type const *add(value_type const *rhs) const noexcept { return self(); }
    inline value_type const *sub(value_type const *rhs) const noexcept { return self(); }
    inline value_type const *mul(value_type const *rhs) const noexcept { return self(); }
    inline value_type const *div(value_type const *rhs) const noexcept { return self(); }

    // unary

    // TODO: return a `value_error` instead
    inline value_type const *neg() const noexcept { return self(); }
};

struct void_type final : value_type
{
    inline static value_type const *self() noexcept
    {
        static void_type vty;
        return &vty;
    }

    // TODO: is this correct?
    inline type const *meet(type const *rhs) const noexcept { return op_not_implemented_type::self(); }

    // binary

    inline value_type const *add(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
    inline value_type const *sub(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
    inline value_type const *mul(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
    inline value_type const *div(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }

    // unary
    inline value_type const *neg() const noexcept { return op_not_implemented_type::self(); }
};

// Section: int types

struct int_ : value_type
{
};

struct int_top final : int_
{
    inline static value_type const *self() noexcept
    {
        static int_top top;
        return &top;
    }

    inline type const *meet(type const *rhs) const noexcept
    {
        if (rhs->as<int_>())
            return rhs;
        // TODO: handle non-int meets
    }

    // impl value_type

    // TODO: fail if type is NOT `int_`
    // TODO: implement
    inline value_type const *add(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_>())
            return rhs;
    }

    inline value_type const *sub(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_>())
            return rhs;
    }

    inline value_type const *mul(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_>())
            return rhs;
    }

    inline value_type const *div(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_>())
            return rhs;
    }

    inline value_type const *neg() const noexcept { return this; }
};

struct int_bot final : int_
{
    inline static value_type const *self() noexcept
    {
        static int_bot bot;
        return &bot;
    }

    // impl type
    inline type const *meet(type const *rhs) const noexcept { return this; }

    // impl value_type

    // TODO: fail if type is NOT `int_`
    inline value_type const *add(value_type const *rhs) const noexcept { return this; }
    inline value_type const *sub(value_type const *rhs) const noexcept { return this; }
    inline value_type const *mul(value_type const *rhs) const noexcept { return this; }
    inline value_type const *div(value_type const *rhs) const noexcept { return this; }

    inline value_type const *neg() const noexcept { return this; }
};

struct int_const final : int_
{
    explicit int_const(int64_t n) noexcept : n{n} {}

    // impl type
    inline type const *meet(type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return (ptr->n == n) ? this : int_bot::self();
        else if (rhs->as<int_top>())
            return this;
    }

    // impl value_type
    inline value_type const *add(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new int_const{n + ptr->n};
        else if (rhs->as<int_top>())
            return this;
    }

    inline value_type const *sub(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new int_const{n - ptr->n};
        else if (rhs->as<int_top>())
            return this;
    }

    inline value_type const *mul(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return new int_const{n * ptr->n};
        else if (rhs->as<int_top>())
            return this;
    }

    inline value_type const *div(value_type const *rhs) const noexcept
    {
        if (rhs->as<int_bot>())
            return rhs;
        else if (auto ptr = rhs->as<int_const>(); ptr)
            return (ptr->n == 0) ? int_top::self() : new int_const{n / ptr->n};
        else if (rhs->as<int_top>())
            return this;
    }

    inline value_type const *neg() const noexcept { return new int_const{-n}; }

    int64_t n;
};

// TODO: two or more possible ranges (eg. [-5, -3] to [3, 5])
// TODO: lt, gt, le, ge
struct int_range final : int_
{
    inline int_range(int64_t lo = INT64_MIN, int64_t hi = INT64_MAX) noexcept
        : lo{lo}, hi{hi} {}

    inline type const *meet(type const *rhs) const noexcept
    {
        if (auto r = rhs->as<int_const>())
        {
            return (lo <= r->n && r->n <= hi) ? this : int_bot::self();
        }
        else
        {
            // TODO: other alternatives
            return int_bot::self();
        }
    }

    int64_t lo, hi;
};

// Section: pointer types

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

    inline type const *meet(type const *rhs) const noexcept
    {
    }

private:
    explicit ptr() noexcept {}
};

struct struct_ : type
{
};

// Section: tuple types

struct tuple : type
{
};

struct tuple_top : tuple
{
};

struct tuple_bot : tuple
{
    inline type const *meet(type const *rhs) const noexcept
    {
        return this; // TODO: error on non-matching types
    }
};

struct tuple_n : tuple
{
    inline tuple_n(size_t n, std::unique_ptr<type const *[]> sub) noexcept
        : n{n}, sub(std::move(sub)) {}

    inline type const *meet(type const *rhs) const noexcept
    {
        if (auto ptr = rhs->as<tuple_n>(); ptr && ptr->n == n)
        {
            // TODO: is this correct?
            auto new_tuple = std::make_unique_for_overwrite<type const *[]>(n);
            for (size_t i{}; i < n; ++i)
            {
                new_tuple[i] = sub[i]->meet(ptr->sub[i]);
            }

            return new tuple_n{n, std::move(new_tuple)};
        }

        return new tuple_bot{};
    }

    size_t n;
    std::unique_ptr<type const *[]> sub;
};
