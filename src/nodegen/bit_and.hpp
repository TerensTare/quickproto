

#pragma once

#include "nodegen/value.hpp"

struct bit_and_node final
{
    entt::entity lhs, rhs;

    inline value_type const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<bit_and_node>);

inline value_type const *bit_and_node::infer(type_storage const &types) const
{
    return types.get(lhs).type->band(types.get(rhs).type);
}

inline entt::entity bit_and_node::emit(builder &bld, value_type const *ty) const
{
    if (ty->is_const())
        return make(bld, value_node{ty});

    entt::entity const ins[]{lhs, rhs};
    return bld.make(node_op::BitAnd, ins);
}