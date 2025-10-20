
#pragma once

#include <span>

// TODO:
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
// - maybe handle operators using a function table, rather than letting the type handle them?
// - rename `value_type` to `type`

// TODO: & &= | |= ^ ^=

struct value_type
{
    // TODO: T: value_type
    template <typename T>
    inline T const *as() const noexcept { return dynamic_cast<T const *>(this); }


    // TODO: recheck this
    // lhs = rhs
    virtual value_type const *assign(value_type const *rhs) const noexcept;

    // binary
    // lhs + rhs
    virtual value_type const *add(value_type const *rhs) const noexcept;
    // lhs - rhs
    virtual value_type const *sub(value_type const *rhs) const noexcept;
    // lhs * rhs
    virtual value_type const *mul(value_type const *rhs) const noexcept;
    // lhs / rhs
    virtual value_type const *div(value_type const *rhs) const noexcept;
    // unary
    // -lhs
    virtual value_type const *neg() const noexcept;
    // !lhs
    virtual value_type const *bnot() const noexcept;

    // comparators
    // TODO: just return a `bool_` here probably?
    // TODO: consider using just `cmp3` instead
    // TODO: is `eq` a bytewise compare by default?
    // lhs == rhs
    virtual value_type const *eq(value_type const *rhs) const noexcept;
    // lhs < rhs
    virtual value_type const *lt(value_type const *rhs) const noexcept;

    // lhs != rhs = !(lhs == rhs)
    inline value_type const *ne(value_type const *rhs) const noexcept { return eq(rhs)->bnot(); }
    // lhs <= rhs = !(lhs > rhs) = !(rhs < lhs)
    inline value_type const *le(value_type const *rhs) const noexcept { return rhs->lt(this)->bnot(); }
    // lhs > rhs = rhs < lhs
    inline value_type const *gt(value_type const *rhs) const noexcept { return rhs->lt(this); }
    // lhs >= rhs = !(lhs < rhs)
    inline value_type const *ge(value_type const *rhs) const noexcept { return lt(rhs)->bnot(); }

    // boolean logic
    // lhs && rhs
    virtual value_type const *logic_and(value_type const *rhs) const noexcept;
    // lhs || rhs
    virtual value_type const *logic_or(value_type const *rhs) const noexcept;

    // lhs(args...)
    virtual value_type const *call(std::span<value_type const *> args) const noexcept;

    // TODO: how do you implement this on `struct`, `func`, `tuple`, etc.?
    virtual char const *name() const noexcept = 0;
};

// signals that this unary operator is not implemented
struct unary_op_not_implemented_type final : value_type
{
    // TODO: these should be values, but not errors, right?
    inline unary_op_not_implemented_type(char const *op, value_type const *sub) noexcept
        : op{op}, sub{sub} {}

    inline char const *name() const noexcept { return "<unary-op-not-implemented>"; }

    // TODO: stronger type for the operator
    char const *op;
    value_type const *sub;
};

// signals that this binary operator is not implemented
struct binary_op_not_implemented_type final : value_type
{
    // TODO: these should be values, but not errors, right?
    inline binary_op_not_implemented_type(char const *op, value_type const *lhs, value_type const *rhs) noexcept
        : op{op}, lhs{lhs}, rhs{rhs} {}

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

    inline char const *name() const noexcept { return "<binary-op-not-implemented>"; }

    // TODO: stronger type for the operator
    char const *op;
    value_type const *lhs;
    value_type const *rhs;
};

inline value_type const *value_type::assign(value_type const *rhs) const noexcept { return new binary_op_not_implemented_type{"=", this, rhs}; }

inline value_type const *value_type::add(value_type const *rhs) const noexcept { return new binary_op_not_implemented_type{"+", this, rhs}; }
inline value_type const *value_type::sub(value_type const *rhs) const noexcept { return new binary_op_not_implemented_type{"-", this, rhs}; }
inline value_type const *value_type::mul(value_type const *rhs) const noexcept { return new binary_op_not_implemented_type{"*", this, rhs}; }
inline value_type const *value_type::div(value_type const *rhs) const noexcept { return new binary_op_not_implemented_type{"/", this, rhs}; }

inline value_type const *value_type::neg() const noexcept { return new unary_op_not_implemented_type{"-", this}; }
inline value_type const *value_type::bnot() const noexcept { return new unary_op_not_implemented_type{"!", this}; }

inline value_type const *value_type::eq(value_type const *rhs) const noexcept { return new binary_op_not_implemented_type{"==", this, rhs}; }
inline value_type const *value_type::lt(value_type const *rhs) const noexcept { return new binary_op_not_implemented_type{"<", this, rhs}; }

inline value_type const *value_type::logic_and(value_type const *rhs) const noexcept { return new binary_op_not_implemented_type{"&&", this, rhs}; }
inline value_type const *value_type::logic_or(value_type const *rhs) const noexcept { return new binary_op_not_implemented_type{"||", this, rhs}; }

// TODO: use a custom error for this
inline value_type const *value_type::call(std::span<value_type const *> args) const noexcept { return new unary_op_not_implemented_type{"()", this}; }
