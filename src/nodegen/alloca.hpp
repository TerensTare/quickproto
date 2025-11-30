
#pragma once

#include "nodegen/value.hpp"

struct alloca_node final
{
    using mem_node = void;

    value_type const *ty; // TODO: should this be a `struct_type` instead?

    inline value_type const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value_type const *ty) const;
};

static_assert(nodegen<alloca_node>);

inline value_type const *alloca_node::infer(type_storage const &types) const
{
    return ty;
}

inline entt::entity alloca_node::emit(builder &bld, value_type const *ty) const
{
    // TODO: implement this correctly
    auto node = bld.make(node_op::Alloca);
    bld.reg.get<node_type>(node).type = ty;
    return node;
}