
#pragma once

#include "nodegen/value.hpp"

struct add_node final
{
    entt::entity lhs, rhs;

    inline value_type const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<add_node>);

inline value_type const *add_node::infer(type_storage const &types) const
{
    return types.get(lhs).type->add(types.get(rhs).type);
}

// TODO: emit the correct `add` depending on the type
inline entt::entity add_node::emit(builder &bld, value_type const *ty) const
{
    if (ty->is_const())
        return make(bld, value_node{ty});

    entt::entity const ins[]{lhs, rhs};
    return bld.make(node_op::Add, ins);
}