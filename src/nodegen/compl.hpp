

#pragma once

#include "nodegen/basic.hpp"

struct compl_node final
{
    entt::entity sub;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<compl_node>);

inline value const *compl_node::infer(type_storage const &types) const
{
    return types.get(sub).type->bcompl();
}

inline entt::entity compl_node::emit(builder &bld, value const *val) const
{
    entt::entity const ins[]{sub};
    return bld.make(val, node_op::UnaryCompl, ins);
}