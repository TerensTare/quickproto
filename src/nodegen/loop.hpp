
#pragma once

#include "nodegen/basic.hpp"

struct loop_node final
{
    using ctrl_node = void;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<loop_node>);

inline value const *loop_node::infer(type_storage const &types) const
{
    // TODO: hack
    return bot_type::self();
}

inline entt::entity loop_node::emit(builder &bld, value const *val) const
{
    return bld.make(val, node_op::Loop);
}