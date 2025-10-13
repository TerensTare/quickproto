
#pragma once

#include <span>
#include "types/type.hpp"

// TODO: && ||
// TODO: & &= | |= ^ ^=

struct value_type : type
{
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
    // TODO: by default is `eq` a bytewise compare?
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
