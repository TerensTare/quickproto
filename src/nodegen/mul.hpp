
#pragma once

#include "nodegen/value.hpp"

struct mul_node final
{
    entt::entity lhs, rhs;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *ty) const;
};

static_assert(nodegen<mul_node>);

inline value const *mul_node::infer(type_storage const &types) const
{
    return types.get(lhs).type->mul(types.get(rhs).type);
}

inline entt::entity mul_node::emit(builder &bld, value const *ty) const
{
    if (ty->is_const())
        return make(bld, value_node{ty});

    entt::entity const ins[]{lhs, rhs};
    // TODO: more cases here
    auto const op = ty->as<int_value>() ? node_op::Mul : node_op::Fmul;
    return bld.make(op, ins);
}