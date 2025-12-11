
#pragma once

#include "nodegen/value.hpp"

// TODO:
// - most things (Load, etc.) can happen in parallel with a Cast long as their do not take the cast as input; implement that
// ^ maybe have a `mem_read` and `mem_write` tag rather than simply `mem_node` and store the info somewhere?
// - does this node actually affect memory?

struct cast_node final
{
    type const *target;
    entt::entity base;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *ty) const;
};

static_assert(nodegen<cast_node>);

inline value const *cast_node::infer(type_storage const &types) const
{
    return types.get(base).type->cast(target);
}

inline entt::entity cast_node::emit(builder &bld, value const *ty) const
{
    // TODO: implement this correctly
    auto node = bld.make(node_op::Cast, std::span(&base, 1));
    bld.reg.get<node_type>(node).type = ty;
    return node;
}