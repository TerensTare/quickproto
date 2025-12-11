
#pragma once

#include "nodegen/basic.hpp"

struct loop_node final
{
    using ctrl_node = void;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *ty) const;
};

static_assert(nodegen<loop_node>);

inline value const *loop_node::infer(type_storage const &types) const
{
    // TODO: hack
    return bot_type::self();
}

inline entt::entity loop_node::emit(builder &bld, value const *ty) const
{
    auto const loop = bld.make(node_op::Loop);
    bld.reg.get<node_type>(loop).type = ty;
    return loop;
}