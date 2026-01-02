
#pragma once

#include "nodegen/basic.hpp"

struct alloca_node final
{
    using mem_write = void;

    type const *ty;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<alloca_node>);

inline value const *alloca_node::infer(type_storage const &types) const
{
    // TODO: recheck this
    return new pointer_value{ty};
}

inline entt::entity alloca_node::emit(builder &bld, value const *val) const
{
    return bld.make(val, node_op::Alloca, {});
}