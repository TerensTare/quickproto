
#pragma once

#include "nodegen/basic.hpp"

struct alloca_node final
{
    using mem_write = void;

    type const *ty; // TODO: should this be a `struct_type` instead?

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<alloca_node>);

inline value const *alloca_node::infer(type_storage const &types) const
{
    // TODO: typecheck here, make sure you can init with the given arguments
    return ty->top();
}

inline entt::entity alloca_node::emit(builder &bld, value const *val) const
{
    // TODO: implement this correctly
    return bld.make(val, node_op::Alloca);
}