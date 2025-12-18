
#pragma once

#include "nodegen/basic.hpp"

struct mul_node final
{
    entt::entity lhs, rhs;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<mul_node>);

inline value const *mul_node::infer(type_storage const &types) const
{
    return types.get(lhs).type->mul(types.get(rhs).type);
}

inline entt::entity mul_node::emit(builder &bld, value const *val) const
{
    entt::entity const ins[]{lhs, rhs};
    // TODO: more cases here
    auto const op = val->as<int_value>() ? node_op::Mul : node_op::Fmul;
    return bld.make(val, op, ins);
}