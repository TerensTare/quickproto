

#pragma once

#include "nodegen/value.hpp"

struct sub_node final
{
    entt::entity lhs, rhs;

    inline value_type const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<sub_node>);

inline value_type const *sub_node::infer(type_storage const &types) const
{
    return types.get(lhs).type->sub(types.get(rhs).type);
}

inline entt::entity sub_node::emit(builder &bld, value_type const *ty) const
{
    if (ty->is_const())
        return make(bld, value_node{ty});

    entt::entity const ins[]{lhs, rhs};
    return bld.make(node_op::Sub, ins);
}