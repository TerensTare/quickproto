

#pragma once

#include "nodegen/basic.hpp"

struct not_node final
{
    entt::entity sub;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<not_node>);

inline value const *not_node::infer(type_storage const &types) const
{
    return types.get(sub).type->bnot();
}

inline entt::entity not_node::emit(builder &bld, value const *val) const
{
    entt::entity const ins[]{sub};
    return bld.make(val, node_op::UnaryNot, ins);
}