
#pragma once

#include "token.hpp"

#include "types/composite.hpp"

// TODO: probably best to have a `top_struct` value rather than having `struct{top...}`
// ^ same reasoning for array
// ^ the "problem" is that when you change a field to a constant, the value becomes a `struct{top..., value, top...}`
// ^ but is it really a problem?

// internal compiler error: member offset should be known at compile time
// TODO: make this more descriptive
struct variable_member_access final : value_error
{
    inline variable_member_access(value const *val, value const *index) : val{val}, index{index} {}

    value const *val;
    value const *index;
};

struct struct_type;

// TODO: do not store member names here, only the type
struct struct_value : composite_value
{
    inline explicit struct_value(struct_type const *type, size_t n_values, value const **values)
        : type{type}, n_values{n_values}, values(values) {}

    inline value const *assign(value const *rhs) const noexcept;
    inline value const *index(value const *i) const noexcept;

    inline value const *phi(value const *other) const noexcept
    {
        // TODO: more concrete `join`
        return other->as<struct_value>()
                   ? this
                   : top_value::self();
    }

    struct_type const *type;
    size_t n_values;
    value const **values; // TODO: the value should "own" member values (should it?)
};

struct member_decl final
{
    hashed_name name;
    type const *ty;
};

struct struct_type final : composite_type
{
    inline struct_type(size_t n_members, std::unique_ptr<member_decl[]> members) noexcept
        : composite_type{n_members}, members(std::move(members)) {}

    inline value const *top() const noexcept
    {
        auto tops = new value const *[n_members];
        for (size_t i{}; i < n_members; ++i)
            tops[i] = members[i].ty->top();

        return new struct_value{this, n_members, tops};
    }

    inline value const *zero() const noexcept
    {
        auto zeros = new value const *[n_members];
        for (size_t i{}; i < n_members; ++i)
            zeros[i] = members[i].ty->zero();

        return new struct_value{this, n_members, zeros};
    }

    // TODO: error if incompatible type
    inline composite_value const *init(value const **values) const noexcept
    {
        return new struct_value{this, n_members, values};
    }

    // TODO: show the actual name of the struct
    inline char const *name() const noexcept { return "{struct}"; }

    std::unique_ptr<member_decl[]> members;
};

// TODO: check matching types first
inline value const *struct_value::assign(value const *rhs) const noexcept
{
    // TODO: implement
    return rhs;
}

// TODO: do something more efficient
inline value const *struct_value::index(value const *i) const noexcept
{
    if (auto ptr = i->as<int_const>())
    {
        if (ptr->n < type->n_members)
            return values[ptr->n];

        // TODO: handle this case out of this function
        return new no_member_error{this, ""};
    }
    else
        return new variable_member_access{this, i};
}
