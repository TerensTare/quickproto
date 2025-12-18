

#pragma once

#include "nodegen/basic.hpp"

// TODO: optimizations (deref addr, etc.)

struct deref_node final
{
    entt::entity sub;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<deref_node>);

inline value const *deref_node::infer(type_storage const &types) const
{
    return types.get(sub).type->deref();
}

inline entt::entity deref_node::emit(builder &bld, value const *val) const
{
    entt::entity const ins[]{sub};
    return bld.make(val, node_op::Deref, ins);
}