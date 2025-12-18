
#pragma once

#include "nodegen/mul.hpp"

struct add_node final
{
    entt::entity lhs, rhs;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<add_node>);

inline value const *add_node::infer(type_storage const &types) const
{
    return types.get(lhs).type->add(types.get(rhs).type);
}

// TODO: emit the correct `add` depending on the type
inline entt::entity add_node::emit(builder &bld, value const *val) const
{
    if (val->is_const())
        return make(bld, value_node{val});

    // a + a == a * 2
    // TODO: only do this for integers/floats, not strings
    if (lhs == rhs)
    {
        auto const two = make(bld, value_node{int_const::make(2)});
        return make(bld, mul_node{lhs, two});
    }

    entt::entity const ins[]{lhs, rhs};
    // TODO: more cases here
    // TODO: use value type, not the hierarchy
    auto const op = val->as<int_value>() ? node_op::Add : node_op::Fadd;
    return bld.make(val, op, ins);
}