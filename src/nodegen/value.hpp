
#pragma once

#include "nodegen/basic.hpp"

struct value_node final
{
    value_type const *ty;

    inline value_type const *infer(type_storage const &) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<value_node>);

inline value_type const *value_node::infer(type_storage const &) const { return ty; }

inline entt::entity value_node::emit(builder &bld, value_type const *ty) const
{
    auto const n = bld.make(node_op::Const);
    bld.reg.get<node_type>(n).type = ty;

    return n;
}