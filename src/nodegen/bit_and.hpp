

#pragma once

#include "nodegen/value.hpp"

struct bit_and_node final
{
    entt::entity lhs, rhs;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<bit_and_node>);

inline value const *bit_and_node::infer(type_storage const &types) const
{
    return types.get(lhs).type->band(types.get(rhs).type);
}

inline entt::entity bit_and_node::emit(builder &bld, value const *val) const
{
    if (val->is_const())
        return make(bld, value_node{val});

    entt::entity const ins[]{lhs, rhs};
    return bld.make(val, node_op::BitAnd, ins);
}