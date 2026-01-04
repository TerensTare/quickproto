

#pragma once

#include "nodegen/basic.hpp"

// TODO: maybe it's best if you split this...

struct phi_node final
{
    entt::entity region;
    entt::entity lhs, rhs;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<phi_node>);

inline value const *phi_node::infer(type_storage const &types) const
{
    return types.get(lhs).type->phi(types.get(rhs).type);
}

inline entt::entity phi_node::emit(builder &bld, value const *val) const
{
    if (lhs == rhs)
        return lhs;

    // TODO: handle case when either branch is null
    // TODO: handle multiple returns
    // TODO: ensure number of expr matches on both
    // TODO: apply optimizations
    entt::entity const ins[]{lhs, rhs};
    auto const ret = bld.make(val, node_op::Phi, ins);
    (void)bld.reg.emplace<region_of_phi>(ret, region);
    return ret; // TODO: fold the `Phi` if possible
}