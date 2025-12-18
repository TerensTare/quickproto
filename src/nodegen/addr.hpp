

#pragma once

#include "nodegen/basic.hpp"

// TODO: optimizations (addr deref, etc.)

struct addr_node final
{
    entt::entity sub;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<addr_node>);

inline value const *addr_node::infer(type_storage const &types) const
{
    return types.get(sub).type->addr();
}

inline entt::entity addr_node::emit(builder &bld, value const *val) const
{
    entt::entity const ins[]{sub};
    return bld.make(val, node_op::Addr, ins);
}