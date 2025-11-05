

#pragma once

#include "nodegen/value.hpp"

struct eq_node final
{
    entt::entity lhs, rhs;

    inline value_type const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<eq_node>);

inline value_type const *eq_node::infer(type_storage const &types) const
{
    return (lhs == rhs)
               ? new bool_const{true}
               : types.get(lhs).type->eq(types.get(rhs).type);
}

// TODO: emit the correct `add` depending on the type
inline entt::entity eq_node::emit(builder &bld, value_type const *ty) const
{
    if (ty->is_const())
        return make(bld, value_node{ty});

    entt::entity const ins[]{lhs, rhs};
    return bld.make(node_op::CmpEq, ins);
}