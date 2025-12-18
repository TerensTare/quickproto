

#pragma once

#include "nodegen/basic.hpp"

struct neg_node final
{
    entt::entity sub;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<neg_node>);

inline value const *neg_node::infer(type_storage const &types) const
{
    return types.get(sub).type->neg();
}

inline entt::entity neg_node::emit(builder &bld, value const *val) const
{
    entt::entity const ins[]{sub};
    return bld.make(val, node_op::UnaryNeg, ins);
}