
#pragma once

#include "builder.hpp"
#include "types/value.hpp"

struct value_node final
{
    value_type const *ty;
};

inline entt::entity make(builder &bld, value_node val) noexcept
{
    auto const n = bld.make(node_op::Const);
    bld.reg.get<node_type>(n).type = val.ty;

    return n;
}