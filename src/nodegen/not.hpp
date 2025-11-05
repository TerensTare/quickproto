

#pragma once

#include "nodegen/value.hpp"

struct not_node final
{
    entt::entity sub;

    inline value_type const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<not_node>);

inline value_type const *not_node::infer(type_storage const &types) const
{
    return types.get(sub).type->bnot();
}

inline entt::entity not_node::emit(builder &bld, value_type const *ty) const
{
    if (ty->is_const())
        return make(bld, value_node{ty});

    entt::entity const ins[]{sub};
    return bld.make(node_op::UnaryNot, ins);
}