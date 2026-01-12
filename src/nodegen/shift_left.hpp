
#pragma once

#include "nodegen/mul.hpp"

struct shift_left_node final
{
    entt::entity lhs, rhs;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<shift_left_node>);

inline value const *shift_left_node::infer(type_storage const &types) const
{
    return types.get(lhs).type->lsh(types.get(rhs).type);
}

inline entt::entity shift_left_node::emit(builder &bld, value const *val) const
{
    entt::entity const ins[]{lhs, rhs};
    // TODO: more cases here
    return bld.make(val, node_op::ShiftLeft, ins);
}