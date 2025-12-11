
#pragma once

#include "nodegen/value.hpp"

struct alloca_node final
{
    using mem_node = void;

    type const *ty; // TODO: should this be a `struct_type` instead?

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *ty) const;
};

static_assert(nodegen<alloca_node>);

inline value const *alloca_node::infer(type_storage const &types) const
{
    // TODO: typecheck here, make sure you can init with the given arguments
    return ty->top();
}

inline entt::entity alloca_node::emit(builder &bld, value const *ty) const
{
    // TODO: implement this correctly
    auto node = bld.make(node_op::Alloca);
    bld.reg.get<node_type>(node).type = ty;
    return node;
}