
#pragma once

#include "nodegen/value.hpp"

// TODO:
// - most things (Load, etc.) can happen in parallel with a Cast long as their do not take the cast as input; implement that
// ^ maybe have a `mem_read` and `mem_write` tag rather than simply `mem_node` and store the info somewhere?
// - does this node actually affect memory?

struct cast_node final
{
    using mem_node = void;

    value_type const *target;
    entt::entity base;

    inline value_type const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<cast_node>);

inline value_type const *cast_node::infer(type_storage const &types) const
{
    return types.get(base).type->cast(target);
}

inline entt::entity cast_node::emit(builder &bld, value_type const *ty) const
{
    // TODO: implement this correctly
    auto node = bld.make(node_op::Cast, std::span(&base, 1));
    bld.reg.get<node_type>(node).type = ty;
    return node;
}