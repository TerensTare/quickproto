

#pragma once

#include "nodegen/value.hpp"

struct logic_and_node final
{
    entt::entity lhs, rhs;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<logic_and_node>);

inline value const *logic_and_node::infer(type_storage const &types) const
{
    return types.get(lhs).type->logic_and(types.get(rhs).type);
}

// TODO: emit the correct `add` depending on the type
inline entt::entity logic_and_node::emit(builder &bld, value const *val) const
{
    if (val->is_const())
        return make(bld, value_node{val});

    entt::entity const ins[]{lhs, rhs};
    return bld.make(val, node_op::LogicAnd, ins);
}