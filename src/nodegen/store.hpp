
#pragma once

#include "nodegen/value.hpp"

struct store_node final
{
    entt::entity mem_state;
    value_type const *lhs;
    entt::entity rhs;

    inline value_type const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<store_node>);

inline value_type const *store_node::infer(type_storage const &types) const
{
    return lhs->assign(types.get(rhs).type);
}

inline entt::entity store_node::emit(builder &bld, value_type const *ty) const
{
    // TODO: optimize `Store`s if possible
    auto const store = bld.make(node_op::Store, std::span(&rhs, 1));
    bld.reg.get<node_type>(store).type = ty;
    bld.reg.emplace<mem_effect>(store).target = mem_state;

    return store;
}