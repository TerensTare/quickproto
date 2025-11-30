
#pragma once

#include "nodegen/mul.hpp"

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

    // a + a == a * 2
    // TODO: only do this for integers/floats, not strings
    if (lhs == rhs)
    {
        auto const two = make(bld, value_node{int_const::value(2)});
        return make(bld, mul_node{lhs, two});
    }

    entt::entity const ins[]{lhs, rhs};
    return bld.make(node_op::Add, ins);
}