
#pragma once

#include "token.hpp"

#include "types/type.hpp"
#include "types/value.hpp"

// internal compiler error: member offset should be known at compile time
// TODO: make this more descriptive
struct variable_member_access final : value_error
{
    inline variable_member_access(value const *ty, value const *index) : ty{ty}, index{index} {}

    inline char const *name() const noexcept { return "<variable-member-access>"; }

    value const *ty;
    value const *index;
};

struct struct_type;

// TODO: do not store member names here, only the type
struct struct_value : value
{
    inline explicit struct_value(struct_type const *type)
        : type{type} {}

    inline value const *index(value const *i) const noexcept;

    // TODO: provide a better name
    inline char const *name() const noexcept { return "<struct>"; }

    struct_type const *type;
    // TODO: you probably need to keep a list of member values for optimizations
};

struct member_decl final
{
    hashed_name name;
    type const *ty;
};

struct struct_type final : type
{
    inline struct_type(size_t n_members, std::unique_ptr<member_decl[]> members) noexcept
        : n_members{n_members}, members(std::move(members)) {}

    inline value const *top() const noexcept { return new struct_value{this}; }

    size_t n_members;
    std::unique_ptr<member_decl[]> members;
};

// TODO: do something more efficient
inline value const *struct_value::index(value const *i) const noexcept
{
    if (auto ptr = i->as<int_const>())
    {
        if (ptr->n < type->n_members)
            return type->members[ptr->n].ty->top();

        // TODO: handle this case out of this function
        return new no_member_error{this, ""};
    }
    else
        return new variable_member_access{this, i};
}
