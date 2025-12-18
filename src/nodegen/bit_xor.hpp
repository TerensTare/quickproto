

#pragma once

#include "nodegen/basic.hpp"

struct bit_xor_node final
{
    entt::entity lhs, rhs;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<bit_xor_node>);

inline value const *bit_xor_node::infer(type_storage const &types) const
{
    return types.get(lhs).type->bxor(types.get(rhs).type);
}

inline entt::entity bit_xor_node::emit(builder &bld, value const *val) const
{
    entt::entity const ins[]{lhs, rhs};
    return bld.make(val, node_op::BitXor, ins);
}