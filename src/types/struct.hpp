
#pragma once

#include "token.hpp"
#include "types/value.hpp"

// internal compiler error: member offset should be known at compile time
// TODO: make this more descriptive
struct variable_member_access final : value_error
{
    inline variable_member_access(value_type const *ty, value_type const *index) : ty{ty}, index{index} {}

    inline char const *name() const noexcept { return "<variable-member-access>"; }

    value_type const *ty;
    value_type const *index;
};

struct member_info final
{
    hashed_name name;
    value_type const *type;
};

// TODO: do not store member names here, only the type
struct struct_type : value_type
{
    inline struct_type(size_t n_members, std::unique_ptr<member_info[]> members)
        : n_members{n_members}, members(std::move(members)) {}

    inline value_type const *index(value_type const *i) const noexcept;

    // TODO: provide a better name
    inline char const *name() const noexcept { return "<struct>"; }

    size_t n_members;
    std::unique_ptr<member_info[]> members;
};

// TODO: do something more efficient
inline value_type const *struct_type::index(value_type const *i) const noexcept
{
    if (auto ptr = i->as<int_const>())
    {
        if (ptr->n < n_members)
            return members[ptr->n].type;

        // TODO: handle this case out of this function
        return new no_member_error{this, ""};
    }
    else
        return new variable_member_access{this, i};
}
