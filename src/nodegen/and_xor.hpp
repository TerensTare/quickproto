

#pragma once

#include "nodegen/compl.hpp"

// HACK: recheck this impl
struct and_xor_node final
{
    entt::entity lhs, rhs;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<and_xor_node>);

inline value const *and_xor_node::infer(type_storage const &types) const
{
    // TODO: recheck this
    return types.get(lhs).type->band(types.get(rhs).type->bcompl());
}

inline entt::entity and_xor_node::emit(builder &bld, value const *val) const
{
    auto const inv = make(bld, compl_node{rhs});
    entt::entity const ins[]{lhs, inv};
    return bld.make(val, node_op::BitAnd, ins);
}