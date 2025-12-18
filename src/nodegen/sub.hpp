

#pragma once

#include "nodegen/basic.hpp"

struct sub_node final
{
    entt::entity lhs, rhs;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<sub_node>);

inline value const *sub_node::infer(type_storage const &types) const
{
    return types.get(lhs).type->sub(types.get(rhs).type);
}

inline entt::entity sub_node::emit(builder &bld, value const *val) const
{
    entt::entity const ins[]{lhs, rhs};
    // TODO: more cases here
    auto const op = val->as<int_value>() ? node_op::Sub : node_op::Fsub;
    return bld.make(val, op, ins);
}