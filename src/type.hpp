
#pragma once

#include <concepts>
#include <cstdint>
#include <memory>
#include <vector>

// TODO:
// - `static value_type::name` for better error messages
// - have a common interface for operators, then each sub-type implements its own version
// ^ eg. `add` is `+`, but `int + int` is `iadd`
// - separate float32 from float64
// ^ `float32_bot` and `float64_bot` are two distinct types; same for top, etc.
// - `top` and `bot` should have a common representation for `value_type` types; and `top` should "bubble out" when encountered
// - `value_type.assign`, a non-customizable call that just sets `lhs = rhs`, but unless `lhs` is NOT assignable from `rhs`
// - what is the type of an error?
// - use a custom typeid solution, rather than dynamic cast
// ^ How do you represent subclassing with indices? (eg. how do you know a type is any kind of `int`?)
// ^ but do you really need subclassing?
// ^ value and control flow nodes have disjoint types, so make separate types somehow
// - signed and unsigned integers should be separate; then you only have these types for integers
// ^ then sized integers are for example: uint8 is `uint_range{0, 255}`
// - maybe use a value_stack and index into it when representing types
// - `control_flow_type`
// - maybe handle operators using a function table, rather than letting the type handle them?

struct bool_;
struct bool_const;
struct int_;

struct type
{
    virtual ~type() {}

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
};

// unknown type (runtime values, etc.)
struct bot final : type
{
    inline static type const *self() noexcept
    {
        static bot bot;
        return &bot;
    }
};

// Section: control flow types

struct flow_type : type
{
    // TODO: is there any other type except `bool_` that might be useful here?
    virtual flow_type const *branch(bool_ const *cond) const noexcept = 0;

    // TODO:
    // - flow_type const *merge() that determines the type of a `Region`
};

// unreachable node
struct not_ctrl final : flow_type
{
    inline static flow_type const *self() noexcept
    {
        static not_ctrl c;
        return &c;
    }

    // unreachable code is always unreachable, no matter the branches you make of it
    inline flow_type const *branch(bool_ const *cond) const noexcept { return this; }
};

// reachable node
struct ctrl final : flow_type
{
    inline static flow_type const *self() noexcept
    {
        static ctrl c;
        return &c;
    }

    inline flow_type const *branch(bool_ const *cond) const noexcept;
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
    // comparators
    // TODO: just return a `bool_` here probably?
    // TODO: implement other ops by these ops + `not`
    // TODO: consider using just `cmp3` instead
    virtual value_type const *eq(value_type const *rhs) const noexcept = 0;
    virtual value_type const *lt(value_type const *rhs) const noexcept = 0;
};

// signals that this operator/overload is not implemented
// TODO: add info about operand types
struct op_not_implemented_type final : value_type
{
    inline static value_type const *self() noexcept
    {
        static op_not_implemented_type ty;
        return &ty;
    }

    // TODO: return a `value_error` instead
    inline value_type const *add(value_type const *rhs) const noexcept { return this; }
    inline value_type const *sub(value_type const *rhs) const noexcept { return this; }
    inline value_type const *mul(value_type const *rhs) const noexcept { return this; }
    inline value_type const *div(value_type const *rhs) const noexcept { return this; }

    // unary

    // TODO: return a `value_error` instead
    inline value_type const *neg() const noexcept { return this; }

    // comparators
    inline value_type const *eq(value_type const *rhs) const noexcept { return this; }
    inline value_type const *lt(value_type const *rhs) const noexcept { return this; }
};

struct void_type final : value_type
{
    inline static value_type const *self() noexcept
    {
        static void_type vty;
        return &vty;
    }

    // binary

    inline value_type const *add(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
    inline value_type const *sub(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
    inline value_type const *mul(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
    inline value_type const *div(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }

    // unary
    inline value_type const *neg() const noexcept { return op_not_implemented_type::self(); }

    // comparators
    inline value_type const *eq(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
    inline value_type const *lt(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
};

// Section: boolean types

struct bool_ : value_type
{
    // binary
    inline value_type const *add(value_type const *rhs) const noexcept final { return op_not_implemented_type::self(); }
    inline value_type const *sub(value_type const *rhs) const noexcept final { return op_not_implemented_type::self(); }
    inline value_type const *mul(value_type const *rhs) const noexcept final { return op_not_implemented_type::self(); }
    inline value_type const *div(value_type const *rhs) const noexcept final { return op_not_implemented_type::self(); }

    // unary
    inline value_type const *neg() const noexcept final { return op_not_implemented_type::self(); }

    // comparators
    inline value_type const *lt(value_type const *rhs) const noexcept final { return op_not_implemented_type::self(); }
};

struct bool_top final : bool_
{
    inline static value_type const *self() noexcept
    {
        static bool_top top;
        return &top;
    }

    // impl value_type

    // comparators
    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        // TODO: is this correct?
        return (rhs->as<bool_>())
                   ? rhs
                   : op_not_implemented_type::self();
    }
};

struct bool_const final : bool_
{
    inline explicit bool_const(bool b) noexcept
        : b{b} {}

    // impl value_type

    // comparators
    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        if (auto rbool = rhs->as<bool_>())
        {
            // TODO: is this correct?
            if (auto r = rbool->as<bool_const>())
                return new bool_const{b == r->b};
            else
                return r->eq(this);
        }
        else
            return op_not_implemented_type::self();
    }

    bool b;
};

struct bool_bot final : bool_
{
    inline static value_type const *self() noexcept
    {
        static bool_bot bot;
        return &bot;
    }

    // impl value_type

    // comparators
    // TODO: handle non-bool meets
    inline value_type const *eq(value_type const *rhs) const noexcept { return this; }
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

    // impl value_type

    inline value_type const *add(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? rhs
                   : op_not_implemented_type::self();
    }

    inline value_type const *sub(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? rhs
                   : op_not_implemented_type::self();
    }

    inline value_type const *mul(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? rhs
                   : op_not_implemented_type::self();
    }

    inline value_type const *div(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? rhs
                   : op_not_implemented_type::self();
    }

    inline value_type const *neg() const noexcept { return this; }

    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? bool_top::self()
                   : op_not_implemented_type::self();
    }

    inline value_type const *lt(value_type const *rhs) const noexcept
    {
        return (rhs->as<int_>())
                   ? bool_top::self()
                   : op_not_implemented_type::self();
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

    // TODO: fail if type is NOT `int_`
    inline value_type const *add(value_type const *rhs) const noexcept { return this; }
    inline value_type const *sub(value_type const *rhs) const noexcept { return this; }
    inline value_type const *mul(value_type const *rhs) const noexcept { return this; }
    inline value_type const *div(value_type const *rhs) const noexcept { return this; }

    inline value_type const *neg() const noexcept { return this; }

    inline value_type const *eq(value_type const *rhs) const noexcept { return this; }
    inline value_type const *lt(value_type const *rhs) const noexcept { return this; }
};

struct int_const final : int_
{
    explicit int_const(int64_t n) noexcept : n{n} {}

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
            return op_not_implemented_type::self();
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
            return op_not_implemented_type::self();
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
            return op_not_implemented_type::self();
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
            return op_not_implemented_type::self();
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
            return op_not_implemented_type::self();
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
            return op_not_implemented_type::self();
    }

    int64_t n;
};

// TODO: two or more possible ranges (eg. [-5, -3] to [3, 5])
// TODO: lt, gt, le, ge
struct int_range final : int_
{
    inline int_range(int64_t lo = INT64_MIN, int64_t hi = INT64_MAX) noexcept
        : lo{lo}, hi{hi} {}

    int64_t lo, hi;
};

// Section: floating point types

struct float_ : value_type
{
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
                   : op_not_implemented_type::self();
    }

    inline value_type const *sub(value_type const *rhs) const noexcept
    {
        return (rhs->as<float_>())
                   ? rhs
                   : op_not_implemented_type::self();
    }

    inline value_type const *mul(value_type const *rhs) const noexcept
    {
        return (rhs->as<float_>())
                   ? rhs
                   : op_not_implemented_type::self();
    }

    inline value_type const *div(value_type const *rhs) const noexcept
    {
        return (rhs->as<float_>())
                   ? rhs
                   : op_not_implemented_type::self();
    }

    inline value_type const *neg() const noexcept { return this; }

    inline value_type const *eq(value_type const *rhs) const noexcept
    {
        return (rhs->as<float_>())
                   ? bool_top::self()
                   : op_not_implemented_type::self();
    }

    inline value_type const *lt(value_type const *rhs) const noexcept
    {
        return (rhs->as<float_>())
                   ? bool_top::self()
                   : op_not_implemented_type::self();
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
            return op_not_implemented_type::self();
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
            return op_not_implemented_type::self();
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
            return op_not_implemented_type::self();
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
            return op_not_implemented_type::self();
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
            return op_not_implemented_type::self();
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
            return op_not_implemented_type::self();
    }

    float f;
};

// TODO: implement float64

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

private:
    explicit ptr() noexcept {}
};

struct struct_ : type
{
};

struct func final : value_type
{
    inline func(value_type const *ret, size_t n_params, std::unique_ptr<value_type const *[]> params) noexcept
        : ret{ret}, n_params{n_params}, params(std::move(params)) {}

    // binary

    inline value_type const *add(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
    inline value_type const *sub(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
    inline value_type const *mul(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
    inline value_type const *div(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }

    // unary
    inline value_type const *neg() const noexcept { return op_not_implemented_type::self(); }

    inline value_type const *eq(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
    inline value_type const *lt(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }

    value_type const *ret;
    size_t n_params;
    std::unique_ptr<value_type const *[]> params;
};

// Section: tuple types

struct tuple : value_type
{
    // TODO: figure these out

    // binary

    inline value_type const *add(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
    inline value_type const *sub(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
    inline value_type const *mul(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
    inline value_type const *div(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }

    // unary
    inline value_type const *neg() const noexcept { return op_not_implemented_type::self(); }

    // comparators
    inline value_type const *eq(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
    inline value_type const *lt(value_type const *rhs) const noexcept { return op_not_implemented_type::self(); }
};

struct tuple_top : tuple
{
};

struct tuple_bot : tuple
{
};

struct tuple_n : tuple
{
    inline tuple_n(size_t n, std::unique_ptr<value_type const *[]> sub) noexcept
        : n{n}, sub(std::move(sub)) {}

    // inline type const *meet(type const *rhs) const noexcept
    // {
    //     // TODO: return something better here
    //     if (!rhs->as<value_type>())
    //         return nullptr;

    //     if (auto ptr = rhs->as<tuple_n>(); ptr && ptr->n == n)
    //     {
    //         // TODO: is this correct?
    //         auto new_tuple = std::make_unique_for_overwrite<value_type const *[]>(n);
    //         for (size_t i{}; i < n; ++i)
    //         {
    //             new_tuple[i] = sub[i]->meet(ptr->sub[i])->as<value_type>();
    //         }

    //         return new tuple_n{n, std::move(new_tuple)};
    //     }

    //     return new tuple_bot{};
    // }

    size_t n;
    std::unique_ptr<value_type const *[]> sub;
};

inline flow_type const *ctrl::branch(bool_ const *cond) const noexcept
{
    // if the condition can be evaluated at compile time and is `true`, this branch is reachable
    if (auto rhs = cond->as<bool_const>(); !rhs || rhs->b)
        return this;
    // otherwise, this branch is unreachable
    return not_ctrl::self();
}