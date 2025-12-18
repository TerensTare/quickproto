

#pragma once

#include "nodegen/basic.hpp"

struct logic_or_node final
{
    entt::entity lhs, rhs;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<logic_or_node>);

inline value const *logic_or_node::infer(type_storage const &types) const
{
    return types.get(lhs).type->logic_or(types.get(rhs).type);
}

// TODO: emit the correct `add` depending on the type
inline entt::entity logic_or_node::emit(builder &bld, value const *val) const
{
    entt::entity const ins[]{lhs, rhs};
    return bld.make(val, node_op::LogicOr, ins);
}