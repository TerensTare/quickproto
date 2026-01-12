

#pragma once

#include "nodegen/basic.hpp"
#include "types/int.hpp"

struct exit_node final
{
    using ctrl_node = void;

    int code = 0;

    inline value const *infer(type_storage const &types) const;
    inline entt::entity emit(builder &bld, value const *val) const;
};

static_assert(nodegen<exit_node>);

inline value const *exit_node::infer(type_storage const &types) const
{
    // HACK: use another type for this node
    return int_value::top();
}

inline entt::entity exit_node::emit(builder &bld, value const *val) const
{
    return bld.make(val, node_op::Exit, {});
}