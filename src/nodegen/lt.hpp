

#pragma once

#include "nodegen/value.hpp"

struct lt_node final
{
    entt::entity lhs, rhs;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<lt_node>);

inline value const *lt_node::infer(type_storage const &types) const
{
    return (lhs == rhs)
               ? new bool_const{false}
               : types.get(lhs).type->lt(types.get(rhs).type);
}

// TODO: emit the correct `add` depending on the type
inline entt::entity lt_node::emit(builder &bld, value const *val) const
{
    if (val->is_const())
        return make(bld, value_node{val});

    entt::entity const ins[]{lhs, rhs};
    return bld.make(val, node_op::CmpLt, ins);
}