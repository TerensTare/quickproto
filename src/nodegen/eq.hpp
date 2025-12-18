

#pragma once

#include "nodegen/basic.hpp"

struct eq_node final
{
    entt::entity lhs, rhs;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<eq_node>);

inline value const *eq_node::infer(type_storage const &types) const
{
    return (lhs == rhs)
               ? bool_const::True()
               : types.get(lhs).type->eq(types.get(rhs).type);
}

// TODO: emit the correct `add` depending on the type
inline entt::entity eq_node::emit(builder &bld, value const *val) const
{
    entt::entity const ins[]{lhs, rhs};
    return bld.make(val, node_op::CmpEq, ins);
}