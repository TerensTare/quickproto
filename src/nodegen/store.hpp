
#pragma once

#include "nodegen/value.hpp"

struct store_node final
{
    using mem_node = void;

    entt::entity lhs, rhs;

    inline value_type const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<store_node>);

inline value_type const *store_node::infer(type_storage const &types) const
{
    return types.get(lhs).type->assign(types.get(rhs).type);
}

inline entt::entity store_node::emit(builder &bld, value_type const *ty) const
{
    // TODO: optimize `Store`s if possible
    entt::entity const ins[]{lhs, rhs};
    auto const store = bld.make(node_op::Store, ins);
    bld.reg.get<node_type>(store).type = ty;
    return store;
}