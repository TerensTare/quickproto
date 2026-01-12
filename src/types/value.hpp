
#pragma once

#include <span>
#include "types/type.hpp"

// TODO:
// - rhs <op> bot is not an error
// - `index_as_const`
// ^ `float32_bot` and `float64_bot` are two distinct types; same for top, etc.
// - `value.assign`, a non-customizable call that just sets `lhs = rhs`, but unless `lhs` is NOT assignable from `rhs`
// - what is the type of an error?
// - use a custom typeid solution, rather than dynamic cast
// ^ How do you represent subclassing with indices? (eg. how do you know a type is any kind of `int`?)
// ^ but do you really need subclassing?
// ^ value and control flow nodes have disjoint types, so make separate types somehow
// - signed and unsigned integers should be separate; then you only have these types for integers
// ^ then sized integers are for example: uint8 is `uint_range{0, 255}`
// - maybe use a value_stack and index into it when representing types
// - maybe handle operators using a function table, rather than letting the type handle them?

struct value
{
    template <typename T>
    inline T const *as() const noexcept
    {
        static_assert(std::derived_from<T, decltype(auto(*this))>);
        return dynamic_cast<T const *>(this);
    }

    // HACK: do something better here
    virtual bool is_const() const { return false; }

    // TODO: recheck this
    // lhs = rhs
    virtual value const *assign(value const *rhs) const noexcept;

    // binary
    // lhs + rhs
    virtual value const *add(value const *rhs) const noexcept;
    // lhs - rhs
    virtual value const *sub(value const *rhs) const noexcept;
    // lhs * rhs
    virtual value const *mul(value const *rhs) const noexcept;
    // lhs / rhs
    virtual value const *div(value const *rhs) const noexcept;

    // bit logic
    // lhs & rhs
    virtual value const *band(value const *rhs) const noexcept;
    // lhs ^ rhs
    virtual value const *bxor(value const *rhs) const noexcept;
    // lhs | rhs
    virtual value const *bor(value const *rhs) const noexcept;

    // lhs << rhs
    virtual value const *lsh(value const *rhs) const noexcept;
    // lhs >> rhs
    virtual value const *rsh(value const *rhs) const noexcept;

    // unary
    // -self
    virtual value const *neg() const noexcept;
    // !self
    virtual value const *bnot() const noexcept;
    // *self
    virtual value const *deref() const noexcept;
    // &self
    // TODO: by default this should not give an error
    // TODO: this should return `typed_memory` probably?
    virtual value const *addr() const noexcept;

    // comparators
    // TODO: just return a `bool_value` here probably?
    // TODO: consider using just `cmp3` instead
    // TODO: is `eq` a bytewise compare by default?
    // lhs == rhs
    virtual value const *eq(value const *rhs) const noexcept;
    // lhs < rhs
    virtual value const *lt(value const *rhs) const noexcept;

    // lhs != rhs = !(lhs == rhs)
    inline value const *ne(value const *rhs) const noexcept { return eq(rhs)->bnot(); }
    // lhs <= rhs = !(lhs > rhs) = !(rhs < lhs)
    inline value const *le(value const *rhs) const noexcept { return rhs->lt(this)->bnot(); }
    // lhs > rhs = rhs < lhs
    inline value const *gt(value const *rhs) const noexcept { return rhs->lt(this); }
    // lhs >= rhs = !(lhs < rhs)
    inline value const *ge(value const *rhs) const noexcept { return lt(rhs)->bnot(); }

    // boolean logic
    // lhs && rhs
    virtual value const *logic_and(value const *rhs) const noexcept;
    // lhs || rhs
    virtual value const *logic_or(value const *rhs) const noexcept;

    // lhs(args...)
    virtual value const *call(std::span<value const *> args) const noexcept;
    // lhs[rhs] or lhs.rhs depending on sub-type
    virtual value const *index(value const *i) const noexcept;

    // target(self)
    virtual value const *cast(type const *target) const noexcept;

    // aka. `join`
    virtual value const *phi(value const *other) const noexcept;
};

// unknown value, used for nodes whose type is lazy-evaluated (eg. calls to not-yet declared functions)
// TODO: do you need to overload the operators for this?
struct top_value final : value
{
    inline static value const *self() noexcept
    {
        static top_value val;
        return &val;
    }

    // TODO: overload other calls too and figure out their type
    // TODO: should lazy nodes be `lazy` or `bot`?

    inline value const *neg() const noexcept { return this; }
    inline value const *unot() const noexcept { return this; }

    inline value const *phi(value const *other) const noexcept { return this; }
};

// no value at all
struct bot_value final : value
{
    inline static value const *self() noexcept
    {
        static bot_value val;
        return &val;
    }

    inline value const *phi(value const *other) const noexcept { return other; }
};

// Error that happens during the typecheck stage.
// TODO: improve the interface of this
struct value_error : value
{
    // TODO: is this correct?
    inline value const *phi(value const *other) const noexcept final { return this; }
};

// signals that this unary operator is not implemented
struct unary_op_not_implemented_type final : value_error
{
    // TODO: these should be values, but not errors, right?
    inline unary_op_not_implemented_type(char const *op, value const *sub) noexcept
        : op{op}, sub{sub} {}

    // TODO: stronger type for the operator
    char const *op;
    value const *sub;
};

// signals that this binary operator is not implemented
struct binary_op_not_implemented_type final : value_error
{
    // TODO: these should be values, but not errors, right?
    inline binary_op_not_implemented_type(char const *op, value const *lhs, value const *rhs) noexcept
        : op{op}, lhs{lhs}, rhs{rhs} {}

    // TODO: return a `value_error` instead
    inline value const *add(value const *rhs) const noexcept { return this; }
    inline value const *sub(value const *rhs) const noexcept { return this; }
    inline value const *mul(value const *rhs) const noexcept { return this; }
    inline value const *div(value const *rhs) const noexcept { return this; }

    // unary

    // TODO: return a `value_error` instead
    inline value const *neg() const noexcept { return this; }

    // comparators
    inline value const *eq(value const *rhs) const noexcept { return this; }
    inline value const *lt(value const *rhs) const noexcept { return this; }

    // TODO: stronger type for the operator
    char const *op;
    value const *lhs;
    value const *rhs;
};

// signals that the given type does not have a member with the given name
struct no_member_error final : value_error
{
    inline no_member_error(value const *base, std::string_view member)
        : base{base}, member{member} {}

    value const *base;
    std::string_view member;
};

inline value const *value::assign(value const *rhs) const noexcept
{
    // TODO: is this correct? not entirely
    return rhs->as<top_value>()
               ? rhs
               : new binary_op_not_implemented_type{"=", this, rhs};
}

inline value const *value::add(value const *rhs) const noexcept
{
    return rhs->as<top_value>()
               ? rhs
               : new binary_op_not_implemented_type{"+", this, rhs};
}

inline value const *value::sub(value const *rhs) const noexcept
{
    return rhs->as<top_value>()
               ? rhs
               : new binary_op_not_implemented_type{"-", this, rhs};
}

inline value const *value::mul(value const *rhs) const noexcept
{
    return rhs->as<top_value>()
               ? rhs
               : new binary_op_not_implemented_type{"*", this, rhs};
}

inline value const *value::div(value const *rhs) const noexcept
{
    return rhs->as<top_value>()
               ? rhs
               : new binary_op_not_implemented_type{"/", this, rhs};
}

inline value const *value::neg() const noexcept { return new unary_op_not_implemented_type{"-", this}; }
inline value const *value::bnot() const noexcept { return new unary_op_not_implemented_type{"!", this}; }
inline value const *value::deref() const noexcept { return new unary_op_not_implemented_type{"*", this}; }
inline value const *value::addr() const noexcept { return new unary_op_not_implemented_type{"&", this}; }

inline value const *value::eq(value const *rhs) const noexcept
{
    return rhs->as<top_value>()
               ? rhs
               : new binary_op_not_implemented_type{"==", this, rhs};
}

inline value const *value::lt(value const *rhs) const noexcept
{
    return rhs->as<top_value>()
               ? rhs
               : new binary_op_not_implemented_type{"<", this, rhs};
}

inline value const *value::logic_and(value const *rhs) const noexcept
{
    return rhs->as<top_value>()
               ? rhs
               : new binary_op_not_implemented_type{"&&", this, rhs};
}

inline value const *value::logic_or(value const *rhs) const noexcept
{
    return rhs->as<top_value>()
               ? rhs
               : new binary_op_not_implemented_type{"||", this, rhs};
}

inline value const *value::band(value const *rhs) const noexcept
{
    return rhs->as<top_value>()
               ? rhs
               : new binary_op_not_implemented_type{"&", this, rhs};
}

inline value const *value::bxor(value const *rhs) const noexcept
{
    return rhs->as<top_value>()
               ? rhs
               : new binary_op_not_implemented_type{"^", this, rhs};
}

inline value const *value::bor(value const *rhs) const noexcept
{
    return rhs->as<top_value>()
               ? rhs
               : new binary_op_not_implemented_type{"|", this, rhs};
}

inline value const *value::lsh(value const *rhs) const noexcept
{
    return rhs->as<top_value>()
               ? rhs
               : new binary_op_not_implemented_type{"<<", this, rhs};
}

inline value const *value::rsh(value const *rhs) const noexcept
{
    return rhs->as<top_value>()
               ? rhs
               : new binary_op_not_implemented_type{">>", this, rhs};
}

inline value const *value::phi(value const *rhs) const noexcept
{
    return top_value::self();
}

// TODO: use a custom error for this
inline value const *value::call(std::span<value const *> args) const noexcept { return new unary_op_not_implemented_type{"()", this}; }
// TODO: use a custom error for this
inline value const *value::index(value const *i) const noexcept { return new binary_op_not_implemented_type{"[]", this, i}; }

inline value const *value::cast(type const *target) const noexcept
{
    // TODO: disable non-defined casts
    // TODO: for constants, this should give out a constant value
    return target->top();
}
